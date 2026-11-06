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
    // 既存の背景やボタンの初期化のほかに、モデルとカメラを初期化
    // ステージは未定のため、ここでは読み込まない（床が無ければ基準位置の高さで接地する）
    const VECTOR slimePosition = GetSlimePosition();

    mTitleModel.Initialize();
    mTitleModel.SetPosition(slimePosition);

    // スライムを映すカメラを準備する（カメラはスライムの位置を基準にする）
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
    DrawString(20, 20, "タイトル", GetColor(255, 255, 255));


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
    // BGM停止
    //Master::mpSoundManager->StopBGM();
}
