#ifndef MULTITANKENEMY_HPP
#define MULTITANKENEMY_HPP
#include "Enemy.hpp"
#include <string>

class MultiTankEnemy : public Enemy {
private:
    int _frame_cnt;
    float _frameTimer = 0.0f;
    int total_hp;
    std::string _frames[3] = {"play/enemy-8.png", "play/enemy-9.png", "play/enemy-11.png"};
public:
    MultiTankEnemy(int x, int y);
    void Update(float);
    void FrameUpdate();
};
#endif