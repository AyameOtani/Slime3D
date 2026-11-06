#include "TitleScene.h"
#include "DxLib.h"
#include "Master.h"
#include "inputManager.h"


TitleScene::TitleScene()
    : Scene()     // 基底クラスのコンストラクタを呼び出しておく

    // メンバ変数として実体化されたボタンを初期化リストで初期化する
    , m_GameStart(
        HOVER_BUTTON_X,
        HOVER_BUTTON_Y,
        0.0f,
        HoverImagePath::GameStartButton,
        ButtonNormalScale,
        ButtonHoverScale
    )
    , m_ExplainGraph(
        HOVER_BUTTON_X,
        HOVER_BUTTON_Y + BUTTON_DIST_Y,
        0.0f,
        HoverImagePath::ExplainButton,
        ButtonNormalScale,
        ButtonHoverScale
    )
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
    UpdateHoverButton();
   
    // 基底クラスの更新処理を呼びだす
    Scene::Update();
}



// ホバーボタンの更新とシーン遷移
void TitleScene::UpdateHoverButton()
{
    m_GameStart.Update();
    m_ExplainGraph.Update();


    // ゲーム開始ボタン
    if (m_GameStart.IsClicked())
    {
        Master::mpGameManager->GetSceneManager()
            ->SetNextScene(SceneManager::SCENE_TYPE::SCENE_GAME);
    }
    // 説明ボタン
    else if (m_ExplainGraph.IsClicked())
    {
        Master::mpGameManager->GetSceneManager()
            ->SetNextScene(SceneManager::SCENE_TYPE::SCENE_EXPLAIN);
    }
}


void TitleScene::Draw()
{
    DrawString(20, 20, "タイトル", GetColor(255, 255, 255));

    // ボタン描画
    m_GameStart.Draw();
    m_ExplainGraph.Draw();


    // 基底クラスの更新処理を呼びだす
    Scene::Draw();
}


void TitleScene::Finalize()
{
    // BGM停止
    //Master::mpSoundManager->StopBGM();
}
