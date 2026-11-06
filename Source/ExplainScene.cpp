#include "ExplainScene.h"
#include "DxLib.h"
#include "Master.h"
#include "inputManager.h"


ExplainScene::ExplainScene()
    : Scene()     // 基底クラスのコンストラクタを呼び出しておく
{

}


ExplainScene::~ExplainScene()
{

}


void ExplainScene::Initialize()
{
}


void ExplainScene::Update()
{

    if (InputManager::CheckDownKey(KEY_INPUT_RETURN))
    {
        Master::mpGameManager->GetSceneManager()
            ->SetNextScene(SceneManager::SCENE_TYPE::SCENE_TITLE);
    }


    // 基底クラスの更新処理を呼びだす
    Scene::Update();
}



void ExplainScene::Draw()
{
    DrawString(20, 20,
        "説明画面\n"
        "将来的には操作画像とかを乗せても良いと思っています "
       " \n遷移確認のためにあるだけです。\n\n"
        "Enterでタイトルです",
        GetColor(255, 255, 255));


    // 基底クラスの更新処理を呼びだす
    Scene::Draw();
}


void ExplainScene::Finalize()
{
    // BGM停止
    //Master::mpSoundManager->StopBGM();
}
