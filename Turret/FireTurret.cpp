#include <allegro5/base.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/color.h>
#include <cmath>
#include <utility>
#include <string>

#include "Bullet/Fire.hpp"
#include "Engine/AudioHelper.hpp"
#include "Engine/Group.hpp"
#include "Engine/Point.hpp"
#include "FireTurret.hpp"
#include "Scene/PlayScene.hpp"
#include "Enemy/Enemy.hpp"

const int FireTurret::Price = 10;
FireTurret::FireTurret(float x, float y) : Turret("play/tower-base.png", "play/turret-5.png", x, y, 100, Price, 0.1) {
    // Move center downward, since we the turret head is slightly biased upward.
    Anchor.y += 8.0f / GetBitmapHeight();
    int tileSize = PlayScene::BlockSize;
    sibling_base = new Engine::Sprite("play/tower-base.png", x, y+2*tileSize);
    sibling_head = new Engine::Sprite("play/turret-5.png", x, y+2*tileSize);
}
void FireTurret::CreateBullet() {
    Engine::Point diff = Engine::Point(cos(Rotation - ALLEGRO_PI / 2), sin(Rotation - ALLEGRO_PI / 2));
    float rotation = atan2(diff.y, diff.x);
    Engine::Point normalized = diff.Normalize();
    int tileSize = PlayScene::BlockSize;
    Engine::Point normal = Engine::Point(-normalized.y, normalized.x);
    // Change bullet position to the front of the gun barrel.
    getPlayScene()->BulletGroup->AddNewObject(new Fire(Position + normalized * 36 - normal * 6, diff, rotation, this));
    getPlayScene()->BulletGroup->AddNewObject(new Fire(Position + normalized * 36 + normal * 6, diff, rotation, this));

    // the reversed fire
    Engine::Point startPos = Position + Engine::Point(0, 2 * tileSize);
    Engine::Point up = Engine::Point(0, -1); // Upward
    Engine::Point upNormal = Engine::Point(-up.y, up.x); // Perpendicular to up (right)
    float upRotation = -ALLEGRO_PI / 2; // Upward

    getPlayScene()->BulletGroup->AddNewObject(new Fire(startPos + upNormal * -6, up, upRotation, this));
    getPlayScene()->BulletGroup->AddNewObject(new Fire(startPos + upNormal * 6, up, upRotation, this));

    AudioHelper::PlayAudio("fire.wav");
}

void FireTurret::Draw() const {
    if (Preview) {
        al_draw_filled_circle(Turret::Position.x, Turret::Position.y, Turret::CollisionRadius, al_map_rgba(0, 255, 0, 50));
    }
    imgBase.Draw();
    Sprite::Draw(); // related to top?
    sibling_base->Draw();
    sibling_head->Draw();
    if (PlayScene::DebugMode) {
        // Draw target radius.
        al_draw_circle(Position.x, Position.y, CollisionRadius, al_map_rgb(0, 0, 255), 2);
    }

}

void FireTurret::Update(float deltaTime) {
    Sprite::Update(deltaTime);
    PlayScene *scene = getPlayScene();
    imgBase.Position = Position;
    imgBase.Tint = Tint;

    // Update sibling sprites' position and tint
    int tileSize = PlayScene::BlockSize;
    if (sibling_base) {
        sibling_base->Position = Position + Engine::Point(0, 2*tileSize);
        sibling_base->Tint = Tint;
    }
    if (sibling_head) {
        sibling_head->Position = Position + Engine::Point(0, 2*tileSize);
        sibling_head->Tint = Tint;
    }

    if (!Enabled)
        return;
    if (Target) {
        // Engine::Point diff = Target->Position - Position;
        // if (diff.Magnitude() > CollisionRadius) {
        //     Target->lockedTurrets.erase(lockedTurretIterator);
        //     Target = nullptr;
        //     lockedTurretIterator = std::list<Turret *>::iterator();
        // }
        int tileSize = PlayScene::BlockSize;
        int turretTileX = static_cast<int>(Position.x) / tileSize;
        int turretTileY = static_cast<int>(Position.y) / tileSize;
        int targetTileX = static_cast<int>(Target->Position.x) / tileSize;
        int targetTileY = static_cast<int>(Target->Position.y) / tileSize;
        if (targetTileX != turretTileX || targetTileY != turretTileY + 1) {
            Target->lockedTurrets.erase(lockedTurretIterator);
            Target = nullptr;
            lockedTurretIterator = std::list<Turret *>::iterator();
            
        }
    }
    if (!Target) {
        // Lock first seen target.
        // Can be improved by Spatial Hash, Quad Tree, ...
        // However simply loop through all enemies is enough for this program.
        for (auto &it : scene->EnemyGroup->GetObjects()) {
            // lock the only enemy down below by one tile
            int tileSize = PlayScene::BlockSize;
            int turretTileX = static_cast<int>(Position.x) / tileSize;
            int turretTileY = static_cast<int>(Position.y) / tileSize;
            int enemyTileX = static_cast<int>(it->Position.x) / tileSize;
            int enemyTileY = static_cast<int>(it->Position.y) / tileSize;

            if (enemyTileX == turretTileX && enemyTileY == turretTileY + 1) {
                // Enemy is in the tile right under the turret
                // You can add your logic here, for example:
                Target = dynamic_cast<Enemy *>(it);
                Target->lockedTurrets.push_back(this);
                lockedTurretIterator = std::prev(Target->lockedTurrets.end());
                break;
            }
        }
    }
    if (Target) {
        // Engine::Point originRotation = Engine::Point(cos(Rotation - ALLEGRO_PI / 2), sin(Rotation - ALLEGRO_PI / 2));
        // Engine::Point targetRotation = (Target->Position - Position).Normalize();
        // float maxRotateRadian = rotateRadian * deltaTime;
        // float cosTheta = originRotation.Dot(targetRotation);
        // // Might have floating-point precision error.
        // if (cosTheta > 1) cosTheta = 1;
        // else if (cosTheta < -1) cosTheta = -1;
        // float radian = acos(cosTheta);
        // Engine::Point rotation;
        // if (abs(radian) <= maxRotateRadian)
        //     rotation = targetRotation;
        // else
        //     rotation = ((abs(radian) - maxRotateRadian) * originRotation + maxRotateRadian * targetRotation) / radian;
        // // Add 90 degrees (PI/2 radian), since we assume the image is oriented upward.
        // Rotation = atan2(rotation.y, rotation.x) + ALLEGRO_PI / 2;
        Rotation = ALLEGRO_PI;
        // Shoot reload.
        reload -= deltaTime;
        if (reload <= 0) {
            // shoot.
            reload = coolDown;
            CreateBullet();
        }
    }
}