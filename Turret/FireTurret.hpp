#ifndef FIRETURRET_HPP
#define FIRETURRET_HPP
#include "Turret.hpp"

class FireTurret : public Turret {
public:
    static const int Price;
    FireTurret(float x, float y);
    void CreateBullet() override;
    void Draw() const override;
    void Update(float deltaTime) override;
    Engine::Sprite *sibling_base;
    Engine::Sprite *sibling_head;
};
#endif   // FIRETURRET_HPP
