#ifndef ScoreboardScene_HPP
#define ScoreboardScene_HPP
#include <memory>
#include <vector>
#include "Engine/IScene.hpp"
#include "Engine/AudioHelper.hpp"
#include "UI/Component/ImageButton.hpp"

struct date_time{
    int month;
    int date;
    int hour;
    int minute;
};

// score: money / 150 * 100 - life / 10 * 100
//        = money*2/3 - life*10
class ScoreboardScene final : public Engine::IScene {
private:
    static std::vector<std::pair<int, date_time>> _record;
    static int _page;
    Engine::ImageButton *next_page_btn, *prev_page_btn;
    Engine::Label *next_word, *prev_word;
    Engine::Label *out_records[5];
    Engine::Label *page_info;
    // to play music
    ALLEGRO_SAMPLE_ID musicId;
public:
    explicit ScoreboardScene() = default;
    void Initialize() override;
    void Terminate() override;
    void NextPageOnClick();
    void PrevPageOnClick();
    void DrawRecord();
    void BackOnClick(int stage);
    void LoadRecords();
    static void AddRecord(int money, int life, int month, int date, int hour, int minute);
};

#endif   // ScoreboardScene_HPP
