#ifndef SHOVEL_HPP
#define SHOVEL_HPP
#include "Turret.hpp"

class Shovel : public Turret {
public:
    static const int Price;
    Shovel(float x, float y);
    void Update(float deltaTime) override;
    void Draw() const override;
    void CreateBullet() override {}
};
#endif   // MACHINEGUNTURRET_HPP

