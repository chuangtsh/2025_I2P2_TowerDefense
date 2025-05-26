#include <functional>
#include <memory>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>

#include "Engine/GameEngine.hpp"
#include "Engine/Point.hpp"
#include "Engine/Resources.hpp"
#include "PlayScene.hpp"
#include "Scene/ScoreboardScene.hpp"
#include "UI/Component/Label.hpp"
#include "Engine/LOG.hpp"


// this is how you define a static variable: you cannot only declare it
std::vector<std::pair<int, date_time>> ScoreboardScene::_record;
int ScoreboardScene::_page = 1;

bool _reccmp(std::pair<int, date_time> a, std::pair<int, date_time> b) {
    return a.first > b.first;
}


void ScoreboardScene::LoadRecords() {
    std::ifstream file("Resource/records.txt");
    if (!file.is_open()) {
        Engine::LOG(Engine::ERROR) << "Failed to open records.txt.";
        return;
    }

    _record.clear(); // Clear any existing records.

    int score, month, date, hour, minute;
    while (file >> score >> month >> date >> hour >> minute) {
        date_time dt = {month, date, hour, minute};
        _record.emplace_back(score, dt);
    }

    file.close();
    Engine::LOG(Engine::INFO) << "Loaded " << _record.size() << " records from records.txt.";
}

void ScoreboardScene::Initialize() {
    musicId = AudioHelper::PlayBGM("HUMBLE.wav");
    int w = Engine::GameEngine::GetInstance().GetScreenSize().x;
    int h = Engine::GameEngine::GetInstance().GetScreenSize().y;
    int halfW = w / 2;
    int halfH = h / 2;

    Engine::ImageButton *btn;
    btn = new Engine::ImageButton("stage-select/dirt.png", "stage-select/floor.png", halfW - 200, halfH * 3 / 2 + 50, 400, 60);
    btn->SetOnClickCallback(std::bind(&ScoreboardScene::BackOnClick, this, 1));
    AddNewControlObject(btn);
    AddNewObject(new Engine::Label("Back", "pirulen.ttf", 32, halfW, halfH * 3 / 2 + 80, 0, 0, 0, 255, 0.5, 0.5));

    prev_page_btn = new Engine::ImageButton("stage-select/c_estern_blue.png", "stage-select/floor.png", halfW + 300, halfH * 3 / 2 + 0, 200, 40);
    prev_page_btn->SetOnClickCallback(std::bind(&ScoreboardScene::PrevPageOnClick, this));
    AddNewControlObject(prev_page_btn);
    prev_word = new Engine::Label("Prev", "pirulen.ttf", 18, halfW+300+200/2, halfH*3/2+40/2, 0, 0, 0, 255, 0.5, 0.5);
    AddNewObject(prev_word);


    next_page_btn = new Engine::ImageButton("stage-select/c_estern_blue.png", "stage-select/floor.png", halfW + 550, halfH * 3 / 2 + 0, 200, 40);
    next_page_btn->SetOnClickCallback(std::bind(&ScoreboardScene::NextPageOnClick, this));
    AddNewControlObject(next_page_btn);
    next_word = new Engine::Label("Next", "pirulen.ttf", 18, halfW+550+200/2, halfH*3/2+40/2, 0, 0, 0, 255, 0.5, 0.5);
    AddNewObject(next_word);    

    page_info = new Engine::Label("place", "pirulen.ttf", 18, halfW+425+200/2, halfH*3/2+80, 255, 255, 255, 255, 0.5, 0.5);
    AddNewObject(page_info);

    Engine::Label *meta_info = new Engine::Label("date", "pirulen.ttf", 46, halfW, halfH/3-45, 255, 255, 255, 255, 0.5, 0.5);
    AddNewObject(meta_info);
    meta_info->Text = "Date   time        score";

    for ( int i = 0; i < 5; i++ ) {
        out_records[i] = new Engine::Label("default rec", "pirulen.ttf", 42, halfW, halfH/3+i*80+60, 255, 255, 255, 255, 0.5, 0.5);
        AddNewObject(out_records[i]);
    }

    LoadRecords();

    sort(_record.begin(), _record.end(), _reccmp);

    //for testing:
    // now everytime you open scoreboard, you crete a whole new 13 records lol
    // for (int i = 0; i < 13; i++) {
    //     int money = 1000 + i * 50; // Increment money by 50 for each record.
    //     int life = 10 + i;         // Increment life by 1 for each record.
    //     int month = (i % 12) + 1;  // Cycle through months (1 to 12).
    //     int date = (i % 28) + 1;   // Cycle through dates (1 to 28).
    //     int hour = i % 24;         // Cycle through hours (0 to 23).
    //     int minute = (i * 5) % 60; // Increment minutes by 5, cycle through (0 to 59).

    //     // Add the record.
    //     AddRecord(money, life, month, date, hour, minute);
    // }

    DrawRecord();

}
void ScoreboardScene::Terminate() {
    AudioHelper::StopBGM(musicId);
    IScene::Terminate();
}
void ScoreboardScene::BackOnClick(int stage) {
    Engine::GameEngine::GetInstance().ChangeScene("stage-select");
}

void ScoreboardScene::DrawRecord() {
    int w = Engine::GameEngine::GetInstance().GetScreenSize().x;
    int h = Engine::GameEngine::GetInstance().GetScreenSize().y;
    int halfW = w / 2;
    int halfH = h / 2;

    // first draw next_page and pre_page button
    int total_page = (_record.size()+4) / 5;

    // draw the page info
    std::ostringstream page_info_oss;
    page_info_oss << _page << " / " << total_page;
    page_info->Text = page_info_oss.str();

    Engine::LOG(Engine::INFO) << "total:" << _record.size() << "_page:" << _page;
    if ( _page > 1 ) {
        prev_page_btn->Enabled = true;
        prev_page_btn->Visible = true;
    }
    if ( _page <= 1) {
        prev_page_btn->Enabled = false;
        prev_page_btn->Visible = false;
    }
    if ( _page < total_page ) {
        next_page_btn->Enabled = true;
        next_page_btn->Visible = true;
    }
    if ( _page >= total_page ){
        next_page_btn->Enabled = false;
        next_page_btn->Visible = false;
    }
    for (int i = 0; i < 5; i++) {
        int record_index = (_page - 1) * 5 + i; // Calculate the correct record index.
        if (record_index < _record.size()) {
            // Format the date, time, and score using std::ostringstream.
            std::ostringstream oss;
            oss << (_record[record_index].second.month < 10? " ":"") 
                << _record[record_index].second.month << "/"
                << (_record[record_index].second.date < 10 ? "0":"")
                << _record[record_index].second.date  << "    "
                << (_record[record_index].second.hour < 10 ? " ":"")
                << _record[record_index].second.hour << ":"
                << (_record[record_index].second.minute < 10 ? "0" : "") // Add leading zero for minutes.
                << _record[record_index].second.minute;

            std::string time_str = oss.str();

            // Format the score aligned to the right.
            std::ostringstream score_oss;
            score_oss << std::setw(10) << std::right << _record[record_index].first;

            // Combine time and score into a single string.
            std::string record_text = time_str + std::string(20 - time_str.length(), ' ') + score_oss.str();

            // Set the formatted text to the label.
            out_records[i]->Text = record_text;
        } else {
            // Clear the label if no record exists for this slot.
            out_records[i]->Text = "";
        }
    }
}

void ScoreboardScene::NextPageOnClick() {
    // Engine::LOG(Engine::INFO) << "Press a next_page\n";
    
    int total_page = (_record.size()+4) / 5;
    // for testing:
    if ( _page < total_page)
        _page++;
    DrawRecord();
}
void ScoreboardScene::PrevPageOnClick() {
    if ( _page > 1 )
        _page--;
    DrawRecord();
}


void ScoreboardScene::AddRecord(int money, int life, int month, int date, int hour, int minute) {
    int score = money*2/3 - (10-life)*10;
    date_time *ntd = new date_time;
    ntd->date = date;
    ntd->hour = hour;
    ntd->minute = minute;
    ntd->month = month;
    _record.push_back(std::make_pair(score, *ntd));
    std::ofstream fl;
    fl.open("Resource/records.txt", std::ios::app);
    if ( fl.is_open() ) {
        fl << score << " "
             << month << " "
             << date << " "
             << hour << " "
             << minute << "\n"; 
        fl.close();
        Engine::LOG(Engine::INFO) << "save a record";
    }
    else {
        Engine::LOG(Engine::ERROR) << "no save";
    }
    sort(_record.begin(), _record.end(), _reccmp);
}
