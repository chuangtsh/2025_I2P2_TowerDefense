#ifndef FIRE_HPP
#define FIRE_HPP
#include "Bullet.hpp"

class Enemy;
class Turret;
namespace Engine {
    struct Point;
}   // namespace Engine

class Fire : public Bullet {
public:
    explicit Fire(Engine::Point position, Engine::Point forwardDirection, float rotation, Turret *parent);
    void OnExplode(Enemy *enemy) override;
    void Update(float deltaTime) override;
};
#endif   // FIREBULLET_HPP
