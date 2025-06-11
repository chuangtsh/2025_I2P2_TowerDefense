#include <allegro5/allegro_primitives.h>
#include <allegro5/color.h>
#include <cmath>
#include <random>
#include <csignal>
#include <string>
#include <vector>

#include "Bullet/Bullet.hpp"
#include "Enemy.hpp"
#include "Engine/AudioHelper.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Group.hpp"
#include "Engine/IScene.hpp"
#include "Engine/LOG.hpp"
#include "Scene/PlayScene.hpp"
#include "Turret/Turret.hpp"
#include "UI/Animation/DirtyEffect.hpp"
#include "UI/Animation/ExplosionEffect.hpp"

PlayScene *Enemy::getPlayScene() {
    return dynamic_cast<PlayScene *>(Engine::GameEngine::GetInstance().GetActiveScene());
}
void Enemy::OnExplode() {
    getPlayScene()->EffectGroup->AddNewObject(new ExplosionEffect(Position.x, Position.y));
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> distId(1, 3);
    std::uniform_int_distribution<std::mt19937::result_type> dist(1, 20);
    for (int i = 0; i < 10; i++) {
        // Random add 10 dirty effects.
        getPlayScene()->GroundEffectGroup->AddNewObject(new DirtyEffect("play/dirty-" + std::to_string(distId(rng)) + ".png", dist(rng), Position.x, Position.y));
    }
}
Enemy::Enemy(std::string img, float x, float y, float radius, float speed, float hp, int money) : Engine::Sprite(img, x, y), speed(speed), hp(hp), money(money) {
    CollisionRadius = radius;
    reachEndTime = 0;
}
void Enemy::Hit(float damage, bool reach_end) {
    hp -= damage;
    if (hp <= 0) {
        OnExplode();
        // Remove all turret's reference to target.
        for (auto &it : lockedTurrets)
            it->Target = nullptr;
        for (auto &it : lockedBullets)
            it->Target = nullptr;
        if ( !reach_end )
            getPlayScene()->EarnMoney(money);
        getPlayScene()->EnemyGroup->RemoveObject(objectIterator);
        AudioHelper::PlayAudio("explosion.wav");
    }
}
void Enemy::UpdatePath(const std::vector<std::vector<int>> &mapDistance) {
    int x = static_cast<int>(floor(Position.x / PlayScene::BlockSize));
    int y = static_cast<int>(floor(Position.y / PlayScene::BlockSize));
    if (x < 0) x = 0;
    if (x >= PlayScene::MapWidth) x = PlayScene::MapWidth - 1;
    if (y < 0) y = 0;
    if (y >= PlayScene::MapHeight) y = PlayScene::MapHeight - 1;
    Engine::Point pos(x, y);
    int num = mapDistance[y][x];
    if (num == -1) {
        num = 0;
        Engine::LOG(Engine::ERROR) << "Enemy path finding error";
    }
    path = std::vector<Engine::Point>(num + 1);
    while (num != 0) {
        std::vector<Engine::Point> nextHops;
        for (auto &dir : PlayScene::directions) {
            int x = pos.x + dir.x;
            int y = pos.y + dir.y;
            if (x < 0 || x >= PlayScene::MapWidth || y < 0 || y >= PlayScene::MapHeight || mapDistance[y][x] != num - 1)
                continue;
            nextHops.emplace_back(x, y);
        }
        // ADD THIS CHECK:
        if (nextHops.empty()) {
            // No valid next hop found - prevent segfault
            Engine::LOG(Engine::DEBUGGING) << "Enemy path finding failed - no valid next hop";
            return;
        }
        // Choose arbitrary one.
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, nextHops.size() - 1);
        pos = nextHops[dist(rng)];
        path[num] = pos;
        num--;
    }
    path[0] = PlayScene::EndGridPoint;
}
void Enemy::Update(float deltaTime) {
    // Pre-calculate the velocity.
    float remainSpeed = speed * deltaTime;
    while (remainSpeed != 0) {
        if (path.empty() || Position.x / PlayScene::BlockSize >= PlayScene::MapWidth-1) {
            // Reach end point.
            Hit(hp, 1);
            getPlayScene()->Hit();
            reachEndTime = 0;
            return;
        }
        Engine::Point target = path.back() * PlayScene::BlockSize + Engine::Point(PlayScene::BlockSize / 2, PlayScene::BlockSize / 2);
        Engine::Point vec = target - Position;
        // Add up the distances:
        // 1. to path.back()
        // 2. path.back() to border
        // 3. All intermediate block size
        // 4. to end point
        reachEndTime = (vec.Magnitude() + (path.size() - 1) * PlayScene::BlockSize - remainSpeed) / speed;
        Engine::Point normalized = vec.Normalize();
        if (remainSpeed - vec.Magnitude() > 0) {
            Position = target;
            path.pop_back();
            remainSpeed -= vec.Magnitude();
        } else {
            Velocity = normalized * remainSpeed / deltaTime;
            remainSpeed = 0;
        }
    }
    Rotation = atan2(Velocity.y, Velocity.x);
    Sprite::Update(deltaTime);

    // CheckTurretCollision();
}

// void SignalHandler(int signal) {
//     if (signal == SIGSEGV) {
//         std::cerr << "Segmentation fault caught! Exiting gracefully." << std::endl;
//         std::exit(signal); // Exit the program
//     }
// }
void Enemy::CheckTurretCollision() {
    PlayScene* scene = getPlayScene();
    if (!scene) return;
    
    // Get my position on the grid
    int gridX = static_cast<int>(Position.x / PlayScene::BlockSize);
    int gridY = static_cast<int>(Position.y / PlayScene::BlockSize);
    Engine::LOG(Engine::DEBUGGING) << "here1";
    
    Engine::LOG(Engine::DEBUGGING) << "cnt:" << scene->TowerGroup->GetObjects().size();
    for (auto it = scene->TowerGroup->GetObjects().begin(); it != scene->TowerGroup->GetObjects().end();it++ ) {
        Engine::LOG(Engine::DEBUGGING) << "here0";
        Turret* turret = dynamic_cast<Turret*>(*it);

        Engine::LOG(Engine::DEBUGGING) << "here1";
        if (turret == nullptr) {
            // ++it;
            // continue;
            continue;
        }
        Engine::LOG(Engine::DEBUGGING) << "here2";
        // Get turret position on grid
        int turretX = static_cast<int>(turret->Position.x / PlayScene::BlockSize);
        int turretY = static_cast<int>(turret->Position.y / PlayScene::BlockSize);
         Engine::LOG(Engine::DEBUGGING) << "here3";
        
        // Check if on the same tile
        if (gridX == turretX && gridY == turretY) {
            // scene->RemoveTurretAt(turret->Position.x, turret->Position.y);
            // Engine::LOG(Engine::DEBUGGING) << "here insde check collision loop";
            // // Create explosion effect
            // scene->EffectGroup->AddNewObject(
            //     new ExplosionEffect(turret->Position.x, turret->Position.y));
            // AudioHelper::PlayAudio("explosion.wav");
            
            // // Reset tile state
            scene->mapState[turretY][turretX] = PlayScene::TileType::TILE_FLOOR;
            
            // // Special case for fire turret
            // if (turretY + 2 < PlayScene::MapHeight && 
            //     scene->mapState[turretY+2][turretX] == PlayScene::TileType::TILE_TURRET_SIBLING) {
            //     scene->mapState[turretY+2][turretX] = PlayScene::TileType::TILE_FLOOR;
            // }
            // if (turretY >= 2 && 
            //     scene->mapState[turretY-2][turretX] == PlayScene::TileType::TILE_TURRET_SIBLING) {
            //     scene->mapState[turretY-2][turretX] = PlayScene::TileType::TILE_FLOOR; 
            // }
            
            // // Remove turret and update iterator
            turret->GetObjectIterator()->first = true;
            scene->TowerGroup->RemoveObject(turret->GetObjectIterator());
            return;
        }
         Engine::LOG(Engine::DEBUGGING) << "here4";
        if ( it == scene->TowerGroup->GetObjects().end() ) {
            return;
        }
         Engine::LOG(Engine::DEBUGGING) << "here6";
    }
    Engine::LOG(Engine::DEBUGGING) << "exist?";
}

void Enemy::Draw() const {
    Sprite::Draw();
    if (PlayScene::DebugMode) {
        // Draw collision radius.
        al_draw_circle(Position.x, Position.y, CollisionRadius, al_map_rgb(255, 0, 0), 2);
    }
}
