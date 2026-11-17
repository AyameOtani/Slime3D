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
    // ステージを先に読み込む（スライムの床判定で Stage::Active() を使うため）
    mTitleStage.Initialize();

    // スライムの初期化と位置設定
    const VECTOR slimePosition = GetSlimePosition();
    mTitleModel.Initialize();
    mTitleModel.SetPosition(slimePosition);

    // スライムを映すカメラを準備する
    // 描画距離を広げて、奥（z=300付近）のステージも映るようにする
    // 初期アングルはキー9と同じアングルBにする（キー8・9・0で切り替え可能）
    mTitleCamera.SetCameraAngle(TITLE_CAMERA_ANGLE);
    mTitleCamera.Initialize(slimePosition);
}


void TitleScene::Update()
{
    UpdateHoverButton();

    // モデルのジャンプ演出を更新
    mTitleModel.Update();

    // スライムの位置に合わせてカメラを更新（描画前に設定を反映する）
    mTitleCamera.Update(GetSlimePosition());

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
    // ステージを先に描画し、その上にスライムを描画する
    mTitleStage.Draw();

    // タイトル用モデルの描画
    mTitleModel.Draw();

    // ボタン描画
    m_GameStart.Draw();
    m_ExplainGraph.Draw();


    // 基底クラスの更新処理を呼びだす
    Scene::Draw();
}


// タイトルのスライムの表示位置
VECTOR TitleScene::GetSlimePosition() const
{
    return VGet(0.0f, 0.0f, SLIME_POS_Z);
}


void TitleScene::Finalize()
{
    // ステージを解放する
    mTitleStage.Finalize();

    // BGM停止
    //Master::mpSoundManager->StopBGM();
}