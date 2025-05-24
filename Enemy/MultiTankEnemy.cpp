#include <allegro5/allegro_primitives.h>
#include <allegro5/color.h>
#include "Scene/PlayScene.hpp"
#include "MultiTankEnemy.hpp"
#include "Engine/Resources.hpp"

MultiTankEnemy::MultiTankEnemy(int x, int y) : Enemy("play/enemy-8.png", x, y, 10, 70, 18, 30) {
    _frame_cnt = 0;
    total_hp = 18;
}

void MultiTankEnemy::Update(float deltaTime) {
    FrameUpdate();
    Enemy::Update(deltaTime);
    // _frameTimer += deltaTime;

    // if ( _frameTimer >= 1.0f) {
    //     _frameTimer -= 1.0f;
    //     FrameUpdate();
    // }

}


void MultiTankEnemy::FrameUpdate() {
    // _frame_cnt++;
    // _frame_cnt = _frame_cnt % 3;
    if ( Enemy::hp <= total_hp/3 )
        bmp = Engine::Resources::GetInstance().GetBitmap(_frames[2]);
    else if ( Enemy::hp <= total_hp/3*2 )
        bmp = Engine::Resources::GetInstance().GetBitmap(_frames[1]);

}