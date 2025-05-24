#include <allegro5/base.h>
#include <random>
#include <string>

#include "Enemy/Enemy.hpp"
#include "Engine/Group.hpp"
#include "Engine/Point.hpp"
#include "Engine/Collider.hpp"
#include "Fire.hpp"
#include "Scene/PlayScene.hpp"
#include "UI/Animation/DirtyEffect.hpp"

class Turret;

Fire::Fire(Engine::Point position, Engine::Point forwardDirection, float rotation, Turret *parent) : Bullet("play/explosion-3.png", 500, 0.5, position, forwardDirection, rotation - ALLEGRO_PI / 2, parent) {
}
void Fire::OnExplode(Enemy *enemy) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(2, 5);
    getPlayScene()->GroundEffectGroup->AddNewObject(new DirtyEffect("play/dirty-1.png", dist(rng), enemy->Position.x, enemy->Position.y));
}

void Fire::Update(float deltaTime) {
    Sprite::Update(deltaTime);
    PlayScene *scene = getPlayScene();
    // Can be improved by Spatial Hash, Quad Tree, ...
    // However simply loop through all enemies is enough for this program.
    for (auto &it : scene->EnemyGroup->GetObjects()) {
        Enemy *enemy = dynamic_cast<Enemy *>(it);
        if (!enemy->Visible)
            continue;
        if (Engine::Collider::IsCircleOverlap(Position, CollisionRadius, enemy->Position, enemy->CollisionRadius)) {
            OnExplode(enemy);
            enemy->Hit(damage);
            getPlayScene()->BulletGroup->RemoveObject(objectIterator);
            return;
        }
    }

    int tileSize = PlayScene::BlockSize;
    // Check what the fire is on
    if ( scene->CheckTileFloor(Position.x/tileSize, Position.y/tileSize) ) {
        getPlayScene()->BulletGroup->RemoveObject(objectIterator);
    }
    // int tileSize = PlayScene::BlockSize;
    // int tileX = static_cast<int>(Position.x) / tileSize;
    // int tileY = static_cast<int>(Position.y) / tileSize;
    // if (tileX >= 0 && tileX < PlayScene::MapWidth && tileY >= 0 && tileY < PlayScene::MapHeight) {
    //     int tileType = scene->mapState[tileY][tileX];
    //     if (tileType == PlayScene::TILE_DIRT) {
    //         Engine::LOG(Engine::INFO) << "Fire is on the path.";
    //     } else {
    //         Engine::LOG(Engine::INFO) << "Fire is on something else.";
    //     }
    // }

    // Check if out of boundary.
    if (!Engine::Collider::IsRectOverlap(Position - Size / 2, Position + Size / 2, Engine::Point(0, 0), PlayScene::GetClientSize()))
        getPlayScene()->BulletGroup->RemoveObject(objectIterator);
}