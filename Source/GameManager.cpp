#include "GameManager.h"
#include "Master.h"

GameManager::GameManager()
{
	mpSceneManager = std::make_unique<SceneManager>();
	mpSoundManager = std::make_unique<SoundManager>();
	mpResourceManager = std::make_unique<ResourceManager>();
	mpFontManager = std::make_unique<FontManager>();
	mpSlimeManager = std::make_unique<SlimeManager>();
	mpTimeManager = std::make_unique<TimeManager>();

	// 初期化
	mpFontManager->Initialize();
	// サウンドの初期化
	mpSoundManager->Initialize();
	mpSceneManager->Initialize();

	// スライムの初期化はここでは行わない
	// シーン側の初期化で行う

	// 時間管理クラスの初期化
	mpTimeManager->Init();
}


GameManager::~GameManager()
{
	// サウンドの終了処理
	mpSoundManager->Finalize();
	// フォントの終了処理
	mpFontManager->Finalize();
	mpSceneManager->Finalize();

	// スライムの終了処理
	mpSlimeManager->Finalize();
}


void GameManager::Update()
{
	// シーンの更新
	Master::mpGameManager->GetSceneManager()->Update();

	// スライムのプール返却処理
	Master::mpGameManager->GetSlimeManager()->ProcessReturnRequests();

	// 時間更新
	Master::mpGameManager->GetTimeManager()->Update();
}

SceneManager* GameManager::GetSceneManager()
{
	return mpSceneManager.get();
}

SoundManager* GameManager::GetSoundManager()
{
	return mpSoundManager.get();
}

ResourceManager* GameManager::GetResourceManager()
{
	return mpResourceManager.get();
}

FontManager* GameManager::GetFontManager()
{
	return mpFontManager.get();
}

SlimeManager* GameManager::GetSlimeManager()
{
	return mpSlimeManager.get();
}

TimeManager* GameManager::GetTimeManager()
{
	return mpTimeManager.get();
}