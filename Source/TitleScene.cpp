#include "TitleScene.h"
#include "DxLib.h"
#include "Master.h"
#include "inputManager.h"


TitleScene::TitleScene()
    : Scene()     // 基底クラスのコンストラクタを呼び出しておく
{
}

TitleScene::~TitleScene()
{

}


void TitleScene::Initialize()
{
}


void TitleScene::Update()
{
    if (InputManager::CheckDownKey(KEY_INPUT_RETURN))
    {
        Master::mpGameManager->GetSceneManager()->SetNextScene(SceneManager::SCENE_TYPE::SCENE_GAME);
    }

    // 基底クラスの更新処理を呼びだす
    Scene::Update();
}


void TitleScene::Draw()
{
    DrawString(20, 20, "タイトル", GetColor(255, 255, 255));

    // 基底クラスの更新処理を呼びだす
    Scene::Draw();
}


void TitleScene::Finalize()
{
    // BGM停止
    //Master::mpSoundManager->StopBGM();
}
