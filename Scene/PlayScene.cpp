#include <algorithm>
#include <allegro5/allegro.h>
#include <cmath>
#include <fstream>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include <ctime>
#include <typeinfo>

#include "Enemy/Enemy.hpp"
#include "Enemy/SoldierEnemy.hpp"
#include "Enemy/PlaneEnemy.hpp"
#include "Enemy/TankEnemy.hpp"
#include "Enemy/MultiTankEnemy.hpp"
#include "Engine/AudioHelper.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Group.hpp"
#include "Engine/LOG.hpp"
#include "Engine/Resources.hpp"
#include "PlayScene.hpp"
#include "Turret/LaserTurret.hpp"
#include "Turret/MachineGunTurret.hpp"
#include "Turret/FireTurret.hpp"
#include "Turret/TurretButton.hpp"
#include "Turret/Shovel.hpp"
#include "UI/Animation/DirtyEffect.hpp"
#include "UI/Animation/Plane.hpp"
#include "UI/Component/Label.hpp"
#include "ScoreboardScene.hpp"


// TODO HACKATHON-4 (1/3): Trace how the game handles keyboard input.
// TODO HACKATHON-4 (2/3): Find the cheat code sequence in this file.
// TODO HACKATHON-4 (3/3): When the cheat code is entered, a plane should be spawned and added to the scene.
// TODO HACKATHON-5 (1/4): There's a bug in this file, which crashes the game when you win. Try to find it.
// TODO HACKATHON-5 (2/4): The "LIFE" label are not updated when you lose a life. Try to fix it.

bool PlayScene::DebugMode = true;
const std::vector<Engine::Point> PlayScene::directions = { Engine::Point(-1, 0), Engine::Point(0, -1), Engine::Point(1, 0), Engine::Point(0, 1) };
const int PlayScene::MapWidth = 20, PlayScene::MapHeight = 13;
const int PlayScene::BlockSize = 64;
const float PlayScene::DangerTime = 7.61;
const Engine::Point PlayScene::SpawnGridPoint = Engine::Point(-1, 0);
const Engine::Point PlayScene::EndGridPoint = Engine::Point(MapWidth, MapHeight - 1);
const std::vector<int> PlayScene::code = {
    ALLEGRO_KEY_UP, ALLEGRO_KEY_UP, ALLEGRO_KEY_DOWN, ALLEGRO_KEY_DOWN,
    ALLEGRO_KEY_LEFT, ALLEGRO_KEY_RIGHT, ALLEGRO_KEY_LEFT, ALLEGRO_KEY_RIGHT,
    ALLEGRO_KEY_B, ALLEGRO_KEY_A, ALLEGRO_KEY_RSHIFT, ALLEGRO_KEY_ENTER
};
Engine::Point PlayScene::GetClientSize() {
    return Engine::Point(MapWidth * BlockSize, MapHeight * BlockSize);
}

int turret_id=0;


void PlayScene::Initialize() {
    mapState.clear();
    keyStrokes.clear();
    ticks = 0;
    deathCountDown = -1;
    lives = 10;
    money = 150;
    SpeedMult = 1;
    // Add groups from bottom to top.
    AddNewObject(TileMapGroup = new Group());
    AddNewObject(GroundEffectGroup = new Group());
    AddNewObject(DebugIndicatorGroup = new Group());
    AddNewObject(TowerGroup = new Group());
    AddNewObject(EnemyGroup = new Group());
    AddNewObject(BulletGroup = new Group());
    AddNewObject(EffectGroup = new Group());
    // Should support buttons.
    AddNewControlObject(UIGroup = new Group());
    ReadMap();
    ReadEnemyWave();
    mapDistance = CalculateBFSDistance();
    ConstructUI();
    imgTarget = new Engine::Image("play/target.png", 0, 0);
    imgTarget->Visible = false;
    preview = nullptr;
    UIGroup->AddNewObject(imgTarget);
    // Preload Lose Scene
    deathBGMInstance = Engine::Resources::GetInstance().GetSampleInstance("astronomia.ogg");
    Engine::Resources::GetInstance().GetBitmap("lose/benjamin-happy.png");
    // Start BGM.
    bgmId = AudioHelper::PlayBGM("play.ogg");

}
void PlayScene::Terminate() {
    AudioHelper::StopBGM(bgmId);
    AudioHelper::StopSample(deathBGMInstance);
    deathBGMInstance = std::shared_ptr<ALLEGRO_SAMPLE_INSTANCE>();
    IScene::Terminate();
}
void PlayScene::Update(float deltaTime) {
    // If we use deltaTime directly, then we might have Bullet-through-paper problem.
    // Reference: Bullet-Through-Paper
    if (SpeedMult == 0)
        deathCountDown = -1;
    else if (deathCountDown != -1)
        SpeedMult = 1;

    ReadEnemyWave();


    // Calculate danger zone.
    std::vector<float> reachEndTimes;
    for (auto &it : EnemyGroup->GetObjects()) {
        reachEndTimes.push_back(dynamic_cast<Enemy *>(it)->reachEndTime);
    }
    // Can use Heap / Priority-Queue instead. But since we won't have too many enemies, sorting is fast enough.
    std::sort(reachEndTimes.begin(), reachEndTimes.end());
    float newDeathCountDown = -1;
    int danger = lives;
    for (auto &it : reachEndTimes) {
        if (it <= DangerTime) {
            danger--;
            if (danger <= 0) {
                // Death Countdown
                float pos = DangerTime - it;
                if (it > deathCountDown) {
                    // Restart Death Count Down BGM.
                    AudioHelper::StopSample(deathBGMInstance);
                    if (SpeedMult != 0)
                        deathBGMInstance = AudioHelper::PlaySample("astronomia.ogg", false, AudioHelper::BGMVolume, pos);
                }
                float alpha = pos / DangerTime;
                alpha = std::max(0, std::min(255, static_cast<int>(alpha * alpha * 255)));
                dangerIndicator->Tint = al_map_rgba(255, 255, 255, alpha);
                newDeathCountDown = it;
                break;
            }
        }
    }
    deathCountDown = newDeathCountDown;
    if (SpeedMult == 0)
        AudioHelper::StopSample(deathBGMInstance);
    if (deathCountDown == -1 && lives > 0) {
        AudioHelper::StopSample(deathBGMInstance);
        dangerIndicator->Tint.a = 0;
    }
    if (SpeedMult == 0)
        deathCountDown = -1;
    for (int i = 0; i < SpeedMult; i++) {
        IScene::Update(deltaTime);
        // Check if we should create new enemy.
        ticks += deltaTime;
        if (enemyWaveData.empty()) {
            if (EnemyGroup->GetObjects().empty()) {
                // Free resources.
                /*delete TileMapGroup;
                delete GroundEffectGroup;
                delete DebugIndicatorGroup;
                delete TowerGroup;
                delete EnemyGroup;
                delete BulletGroup;
                delete EffectGroup;
                delete UIGroup;
                delete imgTarget;*/
                // Win.

                // add record here
                std::time_t now = std::time(nullptr);
                std::tm* localTime = std::localtime(&now);
                int month = localTime->tm_mon + 1;
                int date = localTime->tm_mday;
                int hour = localTime->tm_hour;
                int minute = localTime->tm_min;
                ScoreboardScene::AddRecord(money, this->lives, month, date, hour ,minute);
                Engine::GameEngine::GetInstance().ChangeScene("win");
            }
            continue;
        }
        auto current = enemyWaveData.front();
        int current_col = enemyWaveColumn.front();
        if (ticks < current.second)
            continue;
        ticks -= current.second;
        enemyWaveData.pop_front();
        enemyWaveColumn.pop_front();
        const Engine::Point SpawnCoordinate = Engine::Point(SpawnGridPoint.x * BlockSize + BlockSize / 2, SpawnGridPoint.y * BlockSize + BlockSize / 2);
        Enemy *enemy;
        switch (current.first) {
            case 1:
                EnemyGroup->AddNewObject(enemy = new SoldierEnemy(SpawnCoordinate.x, SpawnCoordinate.y + current_col * BlockSize ));
                break;
            // TODO HACKATHON-3 (2/3): Add your new enemy here.
            case 2:
                EnemyGroup->AddNewObject(enemy = new PlaneEnemy(SpawnCoordinate.x, SpawnCoordinate.y + current_col * BlockSize));
                break;
            case 3:
                EnemyGroup->AddNewObject(enemy = new TankEnemy(SpawnCoordinate.x, SpawnCoordinate.y + current_col * BlockSize));
                break;
            case 4:
                EnemyGroup->AddNewObject(enemy = new MultiTankEnemy(SpawnCoordinate.x, SpawnCoordinate.y + current_col * BlockSize));
                break;
            default:
                continue;
        }
        enemy->UpdatePath(mapDistance);
        // Compensate the time lost.
        enemy->Update(ticks);
    }
    if (preview) {
        preview->Position = Engine::GameEngine::GetInstance().GetMousePosition();
        // To keep responding when paused.
        preview->Update(deltaTime);
    }
}
void PlayScene::Draw() const {
    IScene::Draw();
    if (DebugMode) {
        // Draw reverse BFS distance on all reachable blocks.
        for (int i = 0; i < MapHeight; i++) {
            for (int j = 0; j < MapWidth; j++) {
                if (mapDistance[i][j] != -1) {
                    // Not elegant nor efficient, but it's quite enough for debugging.
                    Engine::Label label(std::to_string(mapDistance[i][j]), "pirulen.ttf", 32, (j + 0.5) * BlockSize, (i + 0.5) * BlockSize);
                    label.Anchor = Engine::Point(0.5, 0.5);
                    label.Draw();
                }
            }
        }
    }
}
void PlayScene::OnMouseDown(int button, int mx, int my) {
    if ((button & 1) && !imgTarget->Visible && preview) {
        // Cancel turret construct.
        UIGroup->RemoveObject(preview->GetObjectIterator());
        preview = nullptr;
    }
    IScene::OnMouseDown(button, mx, my);
}
void PlayScene::OnMouseMove(int mx, int my) {
    IScene::OnMouseMove(mx, my);
    const int x = mx / BlockSize;
    const int y = my / BlockSize;
    if (!preview || x < 0 || x >= MapWidth || y < 0 || y >= MapHeight) {
        imgTarget->Visible = false;
        return;
    }
    imgTarget->Visible = true;
    imgTarget->Position.x = x * BlockSize;
    imgTarget->Position.y = y * BlockSize;
}
void PlayScene::OnMouseUp(int button, int mx, int my) {
    Engine::LOG(Engine::DEBUGGING) << "turret_id=" << turret_id;
    IScene::OnMouseUp(button, mx, my);
    if (!imgTarget->Visible)
        return;
    const int x = mx / BlockSize;
    const int y = my / BlockSize;
    if (button & 1) {
        if ( turret_id == 3 ) {
            // Shovel
            if ( CheckSpaceTurret(x, y) ) {
                Engine::LOG(Engine::DEBUGGING) << "find turret at x, y";

                if ( mapState[y][x] == TILE_TURRET_SIBLING ) {
                    Engine::LOG(Engine::INFO) << "the fire sibling";
                    RemoveTurretAt(x, y-2);
                    mapState[y-2][x] = TILE_FLOOR;
                    RemoveTurretAt(x, y);
                    mapState[y][x] = TILE_FLOOR;
                }
                else {
                    RemoveTurretAt(x, y); 
                    mapState[y][x] = TILE_FLOOR;
                    if ( mapState[y+2][x] = TILE_TURRET_SIBLING ) {
                        RemoveTurretAt(x, y+2);
                        mapState[y+2][x] = TILE_FLOOR;
                    }
                }
                Engine::Sprite *sprite;
                GroundEffectGroup->AddNewObject(sprite = new DirtyEffect("play/turret-fire.png", 1, x * BlockSize + BlockSize / 2, y * BlockSize + BlockSize / 2));
                sprite->Rotation = 0;
                // Purchase.
                EarnMoney(-preview->GetPrice());
                preview->GetObjectIterator()->first = false;
                UIGroup->RemoveObject(preview->GetObjectIterator());
                // To keep responding when paused.
                preview->Update(0);
                // Remove Preview.
                preview = nullptr;
                std::vector<std::vector<int>> map = CalculateBFSDistance();
                mapDistance = map;
                for (auto &it : EnemyGroup->GetObjects())
                    dynamic_cast<Enemy *>(it)->UpdatePath(mapDistance);
                return;
            }
        }
        else if (mapState[y][x] != TILE_TURRET && mapState[y][x] != TILE_TURRET_SIBLING) {
            if (!preview)
                return;
            // Check if valid.
            else {
                if (!CheckSpaceValid(x, y)) {
                    Engine::Sprite *sprite;
                    GroundEffectGroup->AddNewObject(sprite = new DirtyEffect("play/target-invalid.png", 1, x * BlockSize + BlockSize / 2, y * BlockSize + BlockSize / 2));
                    sprite->Rotation = 0;
                    return;
                }
                if ( turret_id == 2 ) {
                    // if (!CheckSpaceValid(x, y+2) || mapState[y][x] == TILE_OCCUPIED || mapState[y][x] == TILE_FIRE_SIBLING) {
                    if (mapState[y+2][x] == TILE_TURRET || mapState[y+2][x] == TILE_TURRET_SIBLING ) {
                        Engine::LOG(Engine::DEBUGGING) << "invalid here";
                        Engine::Sprite *sprite;
                        GroundEffectGroup->AddNewObject(sprite = new DirtyEffect("play/target-invalid.png", 1, x * BlockSize + BlockSize / 2, y * BlockSize + BlockSize / 2));
                        sprite->Rotation = 0;
                        return;
                    }
                }
            }
            // Purchase.
            EarnMoney(-preview->GetPrice());
            // Remove Preview.
            preview->GetObjectIterator()->first = false;
            UIGroup->RemoveObject(preview->GetObjectIterator());
            // Construct real turret.
            preview->Position.x = x * BlockSize + BlockSize / 2;
            preview->Position.y = y * BlockSize + BlockSize / 2;
            preview->Enabled = true;
            preview->Preview = false;
            preview->Tint = al_map_rgba(255, 255, 255, 255);
            TowerGroup->AddNewObject(preview);
            // To keep responding when paused.
            preview->Update(0);
            // Remove Preview.
            preview = nullptr;

            mapState[y][x] = TILE_TURRET;
            if ( turret_id == 2 ) {
                mapState[y+2][x] = TILE_TURRET_SIBLING;
            }
            OnMouseMove(mx, my);
        }
    }
}
void PlayScene::OnKeyDown(int keyCode) {
    IScene::OnKeyDown(keyCode);
    if (keyCode == ALLEGRO_KEY_TAB) {
        DebugMode = !DebugMode;
    } else {
        keyStrokes.push_back(keyCode);
        if (keyStrokes.size() > code.size())
            keyStrokes.pop_front();
        if ( keyStrokes.size() == code.size() ) {
            // Engine::LOG(Engine::VERBOSE) << "validating:\n";
            // for ( auto i : keyStrokes) {
            //     Engine::LOG(Engine::VERBOSE) << i << ' ';
            // }Engine::LOG(Engine::VERBOSE) << "\nnext\n";
            // for (auto i : code) {
            //     Engine::LOG(Engine::VERBOSE)<< i << ' ';
            // } Engine::LOG(Engine::VERBOSE)<< "\n";
            bool same = std::equal(keyStrokes.begin(), keyStrokes.end(), code.begin());
            if ( same ) {
                Engine::LOG(Engine::INFO) << "getting a plane\n";
                EarnMoney(10000);
                EffectGroup->AddNewObject( new Plane() );
            }
        }
    }
    if (keyCode == ALLEGRO_KEY_Q) {
        // Hotkey for MachineGunTurret.
        UIBtnClicked(0);
    } else if (keyCode == ALLEGRO_KEY_W) {
        // Hotkey for LaserTurret.
        UIBtnClicked(1);
    }
    else if (keyCode >= ALLEGRO_KEY_0 && keyCode <= ALLEGRO_KEY_9) {
        // Hotkey for Speed up.
        SpeedMult = keyCode - ALLEGRO_KEY_0;
    }
}
void PlayScene::Hit() {
    lives--;
    // UIGroup->AddNewObject(UILives = new Engine::Label(std::string("Life ") + std::to_string(lives), "pirulen.ttf", 24, 1294, 88));
    UILives->Text = std::string("Life ") + std::to_string(this->lives);
    // UIMoney->Text = std::string("$") + std::to_string(this->money);
    if (lives <= 0) {
        Engine::GameEngine::GetInstance().ChangeScene("lose");
    }
}
int PlayScene::GetMoney() const {
    return money;
}
void PlayScene::EarnMoney(int money) {
    this->money += money;
    UIMoney->Text = std::string("$") + std::to_string(this->money);
}
void PlayScene::ReadMap() {
    // now plants v.s. zombies, so ID = 3
    MapId = 3;
    std::string filename = std::string("Resource/map") + std::to_string(2) + ".txt";
    // Read map file.
    char c;
    std::vector<bool> mapData;
    std::ifstream fin(filename);
    while (fin >> c) {
        Engine::LOG(Engine::DEBUGGING) << "c:" << c;
        switch (c) {
            case '0': mapData.push_back(false); Engine::LOG(Engine::INFO)<<"map0"; break;
            case '1': mapData.push_back(true); Engine::LOG(Engine::INFO)<<"map1";break;
            case '\n':
            case '\r':
                if (static_cast<int>(mapData.size()) / MapWidth != 0)
                    throw std::ios_base::failure("Map data is corrupted.");
                break;
            default: throw std::ios_base::failure("Map data is corrupted.");
        }
    }
    fin.close();
    // Validate map data.
    if (static_cast<int>(mapData.size()) != MapWidth * MapHeight)
        throw std::ios_base::failure("Map data is corrupted.");
    // Store map in 2d array.
    mapState = std::vector<std::vector<TileType>>(MapHeight, std::vector<TileType>(MapWidth));
    for (int i = 0; i < MapHeight; i++) {
        for (int j = 0; j < MapWidth; j++) {
            const int num = mapData[i * MapWidth + j];
            mapState[i][j] = num ? TILE_BASE : TILE_FLOOR;
            if (num)
                TileMapGroup->AddNewObject(new Engine::Image("play/tower-base.png", j * BlockSize, i * BlockSize, BlockSize, BlockSize));
            else
                TileMapGroup->AddNewObject(new Engine::Image("play/floor.png", j * BlockSize, i * BlockSize, BlockSize, BlockSize));
        }
    }
}
void PlayScene::ReadEnemyWave() {
    if (enemyWaveData.size() >= 5) {
        // Skip spawning new enemies if we already have enough in queue
        Engine::LOG(Engine::INFO) << "Enemy queue already has " << enemyWaveData.size() 
                                << " enemies. Skipping new generation.";
        return;
    }
    // Get current game state
    int playerMoney = money;
    int playerLives = lives;
    int currentEnemyCount = EnemyGroup->GetObjects().size();
    
    // Count total turrets and types
    int totalTurrets = 0;
    int machineGunTurrets = 0;
    int laserTurrets = 0;
    int fireTurrets = 0;
    
    // Count turrets per row
    std::vector<int> turretsInRow(MapHeight, 0);
    std::vector<int> mgInRow(MapHeight, 0);
    std::vector<int> laserInRow(MapHeight, 0);
    std::vector<int> fireInRow(MapHeight, 0);
    
    for (auto obj : TowerGroup->GetObjects()) {
        Turret* turret = dynamic_cast<Turret*>(obj);
        if (turret) {
            totalTurrets++;
            // Get turret's row position
            int row = static_cast<int>(turret->Position.y) / BlockSize;
            if (row >= 0 && row < MapHeight) {
                turretsInRow[row]++;
                
                switch (turret->GetType()) {
                    case MACHINEGUN: 
                        machineGunTurrets++; 
                        mgInRow[row]++;
                        break;
                    case LASER: 
                        laserTurrets++; 
                        laserInRow[row]++;
                        break;
                    case FIRE: 
                        fireTurrets++; 
                        fireInRow[row]++;
                        break;
                }
            }
        }
    }
    
    // Calculate difficulty based on game state (0-100 scale)
    float difficulty = 0;
    // More turrets = higher difficulty
    difficulty += std::min(40, totalTurrets * 5);
    // More money = higher difficulty
    difficulty += std::min(20, playerMoney / 20);
    // More lives = higher difficulty
    difficulty += std::min(20, playerLives * 2);
    // Add time-based difficulty progression
    static int waveNumber = 0;
    waveNumber++;
    difficulty += std::min(20, waveNumber);
    
    // Maximum enemies on field based on difficulty
    int maxEnemiesOnField = 10 + static_cast<int>(difficulty / 10);  // 10-20 enemies max
    
    // Adjust spawn count based on current enemy count
    int enemySpaceLeft = maxEnemiesOnField - currentEnemyCount;
    int enemiesToGenerate = std::min(5, std::max(0, enemySpaceLeft));
    
    // If map is already crowded, don't spawn more enemies
    if (enemiesToGenerate <= 0) {
        Engine::LOG(Engine::INFO) << "Max enemies reached (" << currentEnemyCount 
                                 << "/" << maxEnemiesOnField << "). Skipping new spawns.";
        return;
    }
    
    float baseWaitTime = std::max(0.5f, 1.8f - (difficulty / 150.0f)); // 0.5-1.8 seconds
    
    // Log the AI decision process
    Engine::LOG(Engine::INFO) << "Wave " << waveNumber << " - Generating " << enemiesToGenerate 
                             << " enemies (current: " << currentEnemyCount << "/" << maxEnemiesOnField 
                             << "), difficulty: " << difficulty;
    
    // Find weakest and strongest rows
    int weakestRow = 0;
    int strongestRow = 0;
    
    for (int i = 1; i < MapHeight; i++) {
        if (turretsInRow[i] < turretsInRow[weakestRow])
            weakestRow = i;
        if (turretsInRow[i] > turretsInRow[strongestRow])
            strongestRow = i;
    }
    
    // Generate a few enemies based on current state
    for (int i = 0; i < enemiesToGenerate; ++i) {
        int enemyType;
        float waitTime;
        int targetRow;
        
        // Choose target row based on strategy
        int rowStrategy = rand() % 100;
        
        // 60% chance to target weakest row, 20% chance to target strongest row (challenge), 20% chance for random
        if (rowStrategy < 60) {
            targetRow = weakestRow; // Target path of least resistance
        } else if (rowStrategy < 80) {
            targetRow = strongestRow; // Challenge the strongest defense
        } else {
            targetRow = rand() % MapHeight; // Random row
        }
        
        // Early game (waves 1-5)
        if (waveNumber <= 5) {
            // Start with simple enemies
            enemyType = (rand() % 100 < 80) ? 1 : 2; // 80% soldiers, 20% planes
            waitTime = baseWaitTime * 1.2f;
        }
        // Mid game (waves 6-15)
        else if (waveNumber <= 15) {
            // Counter the specific row's defenses
            if (mgInRow[targetRow] > laserInRow[targetRow] && mgInRow[targetRow] > fireInRow[targetRow]) {
                // Many machine guns in this row, send tanks
                enemyType = (rand() % 100 < 60) ? 3 : ((rand() % 100 < 50) ? 2 : 1);
            } 
            else if (laserInRow[targetRow] > mgInRow[targetRow] && laserInRow[targetRow] > fireInRow[targetRow]) {
                // Many lasers in this row, send planes
                enemyType = (rand() % 100 < 60) ? 2 : ((rand() % 100 < 50) ? 1 : 3);
            } 
            else {
                // Many fire turrets or mixed in this row, send mixed enemies
                enemyType = 1 + (rand() % 3);
            }
            waitTime = baseWaitTime * 0.9f;
        }
        // Late game (wave 16+)
        else {
            // Increase challenge progressively
            int randVal = rand() % 100;
            if (randVal < 30) {
                enemyType = 1; // 30% soldiers
            } else if (randVal < 55) {
                enemyType = 2; // 25% planes  
            } else if (randVal < 85) {
                enemyType = 3; // 30% tanks
            } else {
                enemyType = 4; // 15% multi-tanks
            }
            waitTime = baseWaitTime * 0.7f;
        }
        
        // Special cases based on player state
        if (rand() % 100 < 20) {
            if (playerLives <= 3) {
                // Player is low on lives, send easier enemies
                enemyType = (enemyType == 4) ? 3 : ((enemyType == 3) ? 1 : enemyType);
                waitTime *= 1.2f;
            } else if (playerMoney >= 300 && rand() % 100 < 30) {
                // Player is rich, occasionally send expensive enemies
                enemyType = (rand() % 100 < 40) ? 4 : 3;
            }
        }
        
        // If the target row is densely defended, consider stronger enemies
        if (turretsInRow[targetRow] >= 3 && enemyType < 3) {
            // Upgrade to a stronger enemy for heavily defended rows
            enemyType += 1;
        }
        
        // If there are many enemies already, increase wait time
        float crowdFactor = static_cast<float>(currentEnemyCount) / maxEnemiesOnField;
        waitTime *= (1.0f + crowdFactor);
        
        // Add randomness to wait times
        waitTime *= 0.9f + (static_cast<float>(rand() % 40) / 100.0f);
        
        // Add to wave data
        enemyWaveData.emplace_back(enemyType, waitTime);
        enemyWaveColumn.push_back(targetRow); // Store which row this enemy should spawn in
        
        Engine::LOG(Engine::INFO) << "Adding enemy type " << enemyType 
                                 << " targeting row " << targetRow 
                                 << " (turrets in row: " << turretsInRow[targetRow] << ")";
    }
    
    // Occasionally add a mini-boss if player is doing very well
    // But only if there's room for more enemies
    if (waveNumber % 5 == 0 && difficulty > 60 && playerLives > 5 && 
        currentEnemyCount < maxEnemiesOnField - 1) {
        
        // Find the row with the most turrets for an extra challenge
        enemyWaveData.emplace_back(4, baseWaitTime * 0.4f);
        enemyWaveColumn.push_back(strongestRow); // Target the strongest row with the boss
        
        Engine::LOG(Engine::INFO) << "Adding mini-boss: Multi-tank to strongest row " << strongestRow;
    }
    
    Engine::LOG(Engine::INFO) << "Generated " << enemyWaveData.size() << " enemies (total)";
}
// void PlayScene::ReadEnemyWave() {
//     // // change enemy here
//     // // original
//     // std::string filename = std::string("Resource/enemy") + std::to_string(1) + ".txt";
//     // // testing:
//     std::string filename = std::string("Resource/enemy_test.txt");

//     // Read enemy file.
//     float type, wait, repeat;
//     enemyWaveData.clear();
//     std::ifstream fin(filename);
//     while (fin >> type && fin >> wait && fin >> repeat) {
//         for (int i = 0; i < repeat; i++)
//             enemyWaveData.emplace_back(type, wait);
//     }
//     fin.close();
// }
void PlayScene::ConstructUI() {
    // Background
    UIGroup->AddNewObject(new Engine::Image("play/sand.png", 1280, 0, 320, 832));
    // Text
    UIGroup->AddNewObject(new Engine::Label(std::string("Stage ") + std::to_string(MapId), "pirulen.ttf", 32, 1294, 0));
    UIGroup->AddNewObject(UIMoney = new Engine::Label(std::string("$") + std::to_string(money), "pirulen.ttf", 24, 1294, 48));
    UIGroup->AddNewObject(UILives = new Engine::Label(std::string("Life ") + std::to_string(lives), "pirulen.ttf", 24, 1294, 88));
    TurretButton *btn;
    // Button 1
    btn = new TurretButton("play/floor.png", "play/dirt.png",
                           Engine::Sprite("play/tower-base.png", 1294, 136, 0, 0, 0, 0),
                           Engine::Sprite("play/turret-1.png", 1294, 136 - 8, 0, 0, 0, 0), 1294, 136, MachineGunTurret::Price);
    // Reference: Class Member Function Pointer and std::bind.
    btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 0));
    UIGroup->AddNewControlObject(btn);
    // Button 2
    btn = new TurretButton("play/floor.png", "play/dirt.png",
                           Engine::Sprite("play/tower-base.png", 1370, 136, 0, 0, 0, 0),
                           Engine::Sprite("play/turret-2.png", 1370, 136 - 8, 0, 0, 0, 0), 1370, 136, LaserTurret::Price);
    btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 1));
    UIGroup->AddNewControlObject(btn);
    // Button 3
    btn = new TurretButton("play/floor.png", "play/dirt.png",
                           Engine::Sprite("play/tower-base.png", 1446, 136, 0, 0, 0, 0),
                           Engine::Sprite("play/turret-6.png", 1446, 136 - 8, 0, 0, 0, 0), 1446, 136, FireTurret::Price);
    btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 2));
    UIGroup->AddNewControlObject(btn);
    // Tool
    btn = new TurretButton("play/floor.png", "play/dirt.png",
                           Engine::Sprite("play/tool-base.png", 1522, 136, 0, 0, 0, 0),
                           Engine::Sprite("play/shovel.png", 1522, 136 - 8, 0, 0, 0, 0), 1522, 136, Shovel::Price);
    btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 3));
    UIGroup->AddNewControlObject(btn);


    int w = Engine::GameEngine::GetInstance().GetScreenSize().x;
    int h = Engine::GameEngine::GetInstance().GetScreenSize().y;
    int shift = 135 + 25;
    dangerIndicator = new Engine::Sprite("play/benjamin.png", w - shift, h - shift);
    dangerIndicator->Tint.a = 0;
    UIGroup->AddNewObject(dangerIndicator);
}

void PlayScene::UIBtnClicked(int id) {
    turret_id = id;
    if (preview)
        UIGroup->RemoveObject(preview->GetObjectIterator());
    if (id == 0 && money >= MachineGunTurret::Price)
        preview = new MachineGunTurret(0, 0);
    else if (id == 1 && money >= LaserTurret::Price) 
        preview = new LaserTurret(0, 0);
    else if ( id == 2 && money >= FireTurret::Price) {
        preview = new FireTurret(0,0);
    }
    else if ( id == 3 && money >= Shovel::Price ) {
        preview = new Shovel(0,0);
    }
    if (!preview)
        return;
    preview->Position = Engine::GameEngine::GetInstance().GetMousePosition();
    preview->Tint = al_map_rgba(255, 255, 255, 200);
    preview->Enabled = false;
    preview->Preview = true;
    UIGroup->AddNewObject(preview);
    OnMouseMove(Engine::GameEngine::GetInstance().GetMousePosition().x, Engine::GameEngine::GetInstance().GetMousePosition().y);
}

bool PlayScene::CheckSpaceValid(int x, int y) {
    if (x < 0 || x >= MapWidth || y < 0 || y >= MapHeight)
        return false;
    auto map00 = mapState[y][x];
    mapState[y][x] = TILE_TURRET;
    std::vector<std::vector<int>> map = CalculateBFSDistance();
    mapState[y][x] = map00;
    if (map[0][0] == -1)
        return false;
    for (auto &it : EnemyGroup->GetObjects()) {
        Engine::Point pnt;
        pnt.x = floor(it->Position.x / BlockSize);
        pnt.y = floor(it->Position.y / BlockSize);
        if (pnt.x < 0) pnt.x = 0;
        if (pnt.x >= MapWidth) pnt.x = MapWidth - 1;
        if (pnt.y < 0) pnt.y = 0;
        if (pnt.y >= MapHeight) pnt.y = MapHeight - 1;
        if (map[pnt.y][pnt.x] == -1)
            return false;
    }
    // All enemy have path to exit.
    mapState[y][x] = TILE_TURRET;
    mapDistance = map;
    for (auto &it : EnemyGroup->GetObjects())
        dynamic_cast<Enemy *>(it)->UpdatePath(mapDistance);
    return true;
}
bool PlayScene::CheckSpaceTurret(int x, int y ) {
    if (x < 0 || x >= MapWidth || y < 0 || y >= MapHeight)
        return false;
    if ( mapState[y][x] != TILE_TURRET && mapState[y][x] != TILE_TURRET_SIBLING ) {
        return false;
    }
    // // All enemy have path to exit.
    return true;

}
// 6/10 morning start from here
std::vector<std::vector<int>> PlayScene::CalculateBFSDistance() {
    // Plants vs. Zombies logic: distance = steps to rightmost TILE_DIRT in the same row.
    std::vector<std::vector<int>> map(MapHeight, std::vector<int>(MapWidth, -1));
    for (int y = 0; y < MapHeight; ++y) {
        // Find the rightmost TILE_DIRT in this row
        int rightmost = -1;
        for (int x = MapWidth - 1; x >= 0; --x) {
            if (mapState[y][x] == TILE_FLOOR) {
                rightmost = x;
                break;
            }
        }
        if (rightmost == -1) continue; // No path in this row
        // Assign distance for each TILE_DIRT in this row
        for (int x = 0; x <= rightmost; ++x) {
            if (mapState[y][x] == TILE_FLOOR ) {
                map[y][x] = rightmost - x;
            }
        }
        // Optionally, set rightmost tile to 0 (goal)
        // map[y][rightmost] = 0;
    }
    return map;
}
// std::vector<std::vector<int>> PlayScene::CalculateBFSDistance() {
//     // Reverse BFS to find path.
//     std::vector<std::vector<int>> map(MapHeight, std::vector<int>(std::vector<int>(MapWidth, -1)));
//     std::queue<Engine::Point> que;
//     // Push end point.
//     // BFS from end point.
//     if (mapState[MapHeight - 1][MapWidth - 1] != TILE_DIRT)
//         return map;
//     que.push(Engine::Point(MapWidth - 1, MapHeight - 1));
//     map[MapHeight - 1][MapWidth - 1] = 0;
//     while (!que.empty()) {
//         Engine::Point p = que.front();
//         que.pop();
//         for (auto dir : directions) {
//             int nx = p.x + dir.x;
//             int ny = p.y + dir.y;

//             // Check bounds and if the tile is empty and not visited.
//             if (nx >= 0 && nx < MapWidth && ny >= 0 && ny < MapHeight &&
//                 mapState[ny][nx] == TILE_DIRT && map[ny][nx] == -1) {
//                 map[ny][nx] = map[p.y][p.x] + 1; // Update distance.
//                 que.push(Engine::Point(nx, ny)); // Add to queue.
//             }
//         }
//         // TODO PROJECT-1 (1/1): Implement a BFS starting from the most right-bottom block in the map.
//         //               For each step you should assign the corresponding distance to the most right-bottom block.
//         //               mapState[y][x] is TILE_DIRT if it is empty.
//     }
//     return map;
// }

// PlayScene& PlayScene::GetInstance() {
//     static PlayScene instance;
//     return instance;
// }
bool PlayScene::CheckTileFloor(int x, int y) {
    return mapState[y][x] == TILE_FLOOR;
}

// not sure about the remove logic
int PlayScene::RemoveTurretAt(int x, int y) {
    Engine::LOG(Engine::DEBUGGING) << "start RemoveTurretAt";
    Engine::LOG(Engine::DEBUGGING) << "=== TowerGroup contents ===";
    int idx = 0;
    int tp = 0;
    for (auto obj : TowerGroup->GetObjects()) {
        Turret* turret = dynamic_cast<Turret*>(obj);
        if (turret) {
            Engine::LOG(Engine::DEBUGGING)
                << "[" << idx << "] "
                << "Type: " << typeid(*turret).name()
                << ", Pos: (" << turret->Position.x << ", " << turret->Position.y << ")"
                << ", Ptr: " << turret;
        } else {
            Engine::LOG(Engine::DEBUGGING)
                << "[" << idx << "] "
                << "Non-turret object, Ptr: " << obj;
        }
        ++idx;
    }
    Engine::LOG(Engine::DEBUGGING) << "=== End of TowerGroup ===";
    // Calculate the center position of the tile
    float px = x * BlockSize + BlockSize / 2;
    float py = y * BlockSize + BlockSize / 2;


    // Find the turret at this position
    for (auto it = TowerGroup->GetObjects().begin(); it != TowerGroup->GetObjects().end(); ++it) {
        Turret* turret = dynamic_cast<Turret*>(*it);
        if (turret && std::abs(turret->Position.x - px) < 1e-3 && std::abs(turret->Position.y - py) < 1e-3) {
            tp = turret->GetType();
            turret->GetObjectIterator()->first = false;
            TowerGroup->RemoveObject(turret->GetObjectIterator());
            mapState[y][x] = TILE_FLOOR;
            break;
        }
    }
    // for ( auto it : TowerGroup->GetObjects() ) {
    //     Turret* turret = dynamic_cast<Turret*>(it);
    //     if (turret && std::abs(turret->Position.x - px) < 1e-3 && std::abs(turret->Position.y - py) < 1e-3 ) {
    //         Engine::LOG(Engine::DEBUGGING) << "inside delete";
    //         tp = turret->GetType();
    //         it = TowerGroup->RemoveObject(turret->GetObjectIterator());
    //         mapState[y][x] = TILE_FLOOR; // Mark the tile as empty
    //         break;
    //     }
    //     Engine::LOG(Engine::DEBUGGING) << "finish delete";
    // }
    Engine::LOG(Engine::DEBUGGING) << "finish remove";
    Engine::LOG(Engine::DEBUGGING) << "=== TowerGroup contents ===";
    idx = 0;
    for (auto obj : TowerGroup->GetObjects()) {
        Turret* turret = dynamic_cast<Turret*>(obj);
        if (turret) {
            Engine::LOG(Engine::DEBUGGING)
                << "[" << idx << "] "
                << "Type: " << typeid(*turret).name()
                << ", Pos: (" << turret->Position.x << ", " << turret->Position.y << ")"
                << ", Ptr: " << turret;
        } else {
            Engine::LOG(Engine::DEBUGGING)
                << "[" << idx << "] "
                << "Non-turret object, Ptr: " << obj;
        }
        ++idx;
    }
    Engine::LOG(Engine::DEBUGGING) << "=== End of TowerGroup ===";
    return tp;
}