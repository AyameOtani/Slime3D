#include "TitleScene.h"
#include "Utility.h" // 呼び出すと、SCREEN_WIDTHとかを使える
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
    // 基底クラスの更新処理を呼びだす
    Scene::Update();
}


void TitleScene::Draw()
{

    // 移動とジャンプの操作説明
    DrawString(200, 20, "タイトル", GetColor(255, 255, 255));

    // 基底クラスの更新処理を呼びだす
    Scene::Draw();
}


void TitleScene::Finalize()
{
    // BGM停止
    //Master::mpSoundManager->StopBGM();
}
