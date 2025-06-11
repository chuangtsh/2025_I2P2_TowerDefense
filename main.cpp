// [main.cpp]
// This is the entry point of your game.
// You can register your scenes here, and start the game.
#include <csignal>
#include <iostream>
#include "Engine/GameEngine.hpp"
#include "Engine/LOG.hpp"
#include "Scene/LoseScene.hpp"
#include "Scene/PlayScene.hpp"
#include "Scene/StageSelectScene.hpp"
#include "Scene/WinScene.hpp"
#include "Scene/StartScene.h"
#include "Scene/SettingsScene.hpp"
#include "Scene/ScoreboardScene.hpp"


int main(int argc, char **argv) {
    // Engine::LOG::SetConfig(true); // no output verbose
	Engine::LOG::SetConfig(true, true); // can output verbose(i.e. keyboard, mouse)

	Engine::GameEngine& game = Engine::GameEngine::GetInstance();

    // TODO HACKATHON-2 (2/3): Register Scenes here
    game.AddNewScene("stage-select", new StageSelectScene());
	game.AddNewScene("play", new PlayScene());
	game.AddNewScene("lose", new LoseScene());
	game.AddNewScene("win", new WinScene());
	game.AddNewScene("start", new StartScene());
	game.AddNewScene("settings", new SettingsScene());
	game.AddNewScene("scoreboard-scene", new ScoreboardScene());

	game.Start("start", 60, 1600, 832);
	return 0;
}
