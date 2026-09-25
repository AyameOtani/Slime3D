#pragma once

#include <memory>

#include "SceneManager.h"
#include "SoundManager.h"
#include "ResourceManager.h"
#include "FontManager.h"

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

private:

	std::unique_ptr<SceneManager> mpSceneManager;
	std::unique_ptr<SoundManager> mpSoundManager;
	std::unique_ptr<ResourceManager> mpResourceManager;
	std::unique_ptr<FontManager> mpFontManager;
};