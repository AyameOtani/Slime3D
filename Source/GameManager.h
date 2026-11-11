#pragma once

#include <memory>

#include "SceneManager.h"
#include "SoundManager.h"
#include "ResourceManager.h"
#include "FontManager.h"
#include "SlimeManager.h"

// ゲームで使用するManagerをまとめて管理するクラス
// Masterで呼ぶことによって全て使えるようにかつ、コードがすっきりする。 大谷
class GameManager
{
public:

	GameManager();
	~GameManager();

	void Update();

	SceneManager* GetSceneManager();
	SoundManager* GetSoundManager();
	ResourceManager* GetResourceManager();
	FontManager* GetFontManager();

	SlimeManager* GetSlimeManager();

private:

	std::unique_ptr<SceneManager> mpSceneManager;
	std::unique_ptr<SoundManager> mpSoundManager;
	std::unique_ptr<ResourceManager> mpResourceManager;
	std::unique_ptr<FontManager> mpFontManager;

	// スライム管理クラス  山本
	std::unique_ptr<SlimeManager> mpSlimeManager;
};