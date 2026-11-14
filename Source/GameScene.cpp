#include "GameScene.h"
#include "Utility.h" // 呼び出すと、SCREEN_WIDTHとかを使える
#include "DxLib.h"
#include "Camera.h"
#include "Stage.h"
#include "Master.h"
#include "inputManager.h"
#include "SlimeModel.h"
#include <cassert>
#include <vector>


// ============================================
// 地面や壁など、ステージを構成する箱を描画する
// ============================================
// boxes：箱の位置や色が登録されている一覧
// const：この関数では一覧の内容を変更しない
// &    ：一覧をコピーせず、元のデータを参照する
void DrawWorld(const std::vector<BoxCollider>& boxes)
{
	// 一覧から箱を1個ずつ取り出して描画する
	for (const BoxCollider& box : boxes)
	{
		// minPosとmaxPos：箱の範囲を決める、向かい合った2つの角の座標
		// box.color     ：箱の基本色
		// GetColor(...) ：光の反射に使う色
		// TRUE          ：線だけでなく、面を塗りつぶして描画する
		DrawCube3D(box.minPos, box.maxPos, box.color, GetColor(180, 180, 180), TRUE);
	}
}



GameScene::GameScene()
	: Scene()     // 基底クラスのコンストラクタを呼び出しておく
{
}

GameScene::~GameScene()
{

}

bool GameScene::SlimeInitialize()
{
    // 形状変化（モーフィング）に使用する3つのモデルを読み込む
   // Resourceフォルダ内のMQOファイルを指定している
   // 戻り値をloadedに保存し、読み込みに成功したかを確認する
    //bool loaded = slime.LoadMorphModels(
    //    "Resource\\Slime\\ibitu1.mqo",
    //    "Resource\\Slime\\ibitu2.mqo",
    //    "Resource\\Slime\\slime.mqo");

    // !loadedは「loadedがfalseの場合」という意味
    //if (!loaded)
    //{
    //    // 読み込みに失敗した場合、確認すべき内容を表示する
    //    // \nは、メッセージ内で改行するための記号
    //    // この部分には終了処理がないので、表示後も処理は続く
    //    MessageBox(nullptr,
    //        "MQOファイルの一部を読み込めませんでした。\n"
    //        "Resourceフォルダと、各モデルの頂点数713個を確認してください。",
    //        "MQO読み込みエラー", MB_OK);

    //    return false;
    //}

    return true;
}

void GameScene::Initialize()
{
    // スライムを初期状態にする
    // 具体的な初期化処理はSlimeModel側に書かれている
    slime.Initialize();

    SlimeInitialize();

    // ============================================
    // スライム管理クラスの初期化  山本
    // ============================================
    Master::mpGameManager->GetSlimeManager()->Setup();

    // ============================================
    // カメラのセットアップ  山本
    // ============================================

    // 横スクロールカメラ
    m_Camera.Initialize(slime.GetPosition());
    m_Camera.SetFollowRate(0.045f);
    m_Camera.SetControlMode(Camera::CONTROL_MODE::TPS); //TPSモードに
    m_Camera.SetTPSDistance(10.0f);

    // ============================================
    // プレイヤーのセットアップ
    // ============================================
    m_pPlayer =  new Player();
    m_pPlayer->Init();

    // ============================================
    // ステージの作成  山本
    // ============================================
    //
    // ステージの読み込み。見た目・当たり判定・塗装をStageで管理する。
    //
    const float STAGE_SCALE = 1.0f;
    if (!m_Stage.Initialize("Resource\\Stage\\stage01.mqo", STAGE_SCALE))
    {
        // 真っ黒な画面で終了しないよう、読み込み失敗の理由を表示する。
        while (ProcessMessage() == 0 && !CheckHitKey(KEY_INPUT_ESCAPE))
        {
            ClearDrawScreen();
            DrawString(20, 90, m_Stage.GetError().c_str(), GetColor(255, 255, 255));
            ScreenFlip();
        }
        m_Stage.Finalize();
        DxLib_End();
        assert(false);
        return ;
    }
    boxes = m_Stage.GetBoxes();
   

    SlimeDef::SlimeSpawnParams spawn;
    spawn.position = VGet(0.0f, 0.0f, 0.0f);
    spawn.type = SlimeDef::SLIME_TYPE::NORMAL;
    spawn.splitPhase = SlimeDef::SLIME_SPLIT_PHASE::PHASE_0;
    Master::mpGameManager->GetSlimeManager()->SpawnSlime(spawn);
}


void GameScene::Update()
{
    // --------------------------------------------
    // スライムの状態を更新する
    // --------------------------------------------

    // ステージの箱の一覧を渡して、スライムの状態を更新する
    // 移動や衝突などの具体的な処理はSlimeModel::Update側にある
    slime.Update(boxes);
    
    // --------------------------------------------
    // ステージを更新する  山本
    // --------------------------------------------
    m_Stage.Update(slime);

    // --------------------------------------------
    // カメラの位置と向きを決める
    // --------------------------------------------
    m_Camera.Update(m_pPlayer->GetPosition());

    float deltaTime = Master::mpGameManager->GetTimeManager()->get_DeltaTime();
    VECTOR cameraForward = m_Camera.GetLookForward();   // カメラの向き

    // プレイヤーの移動パラメータ設定
    m_pPlayer->SetMoveParameters(cameraForward, deltaTime);


	// 基底クラスの更新処理を呼びだす
	Scene::Update();

    // カメラの位置を新しいプレイヤーの座標で設定しなおす
    m_Camera.RefreshFollowPosition(m_pPlayer->GetPosition());
}


void GameScene::Draw()
{
    // --------------------------------------------
    // 3D空間を描画する
    // --------------------------------------------
    // スライムを描画する
    // 描画処理にもステージの箱の一覧を渡している
    slime.Draw(boxes);

    // ステージの描画 山本
    m_Stage.Draw();

    // --------------------------------------------
    // 操作説明を画面に表示する
    // --------------------------------------------
    // DrawStringの最初の2つの値は、画面上の表示位置
    // 画面左上が(0, 0)で、右へ行くほどX、下へ行くほどYが増える
    // GetColorは、赤・緑・青を0～255で指定して色を作る

    // 移動とジャンプの操作説明
    DrawString(20, 20, "WASD : 移動 / SPACE : ジャンプ", GetColor(255, 255, 255));



    // 操作説明
    DrawString(20, 60, "A / D : 左右移動   W / S : 奥・手前   SPACE : ジャンプ", GetColor(255, 255, 255));
    DrawString(20, 85, "Kを押しながら移動 : 通った部分を虹色に塗る   ESC : 終了", GetColor(255, 190, 220));
    DrawString(20, 115, "1 : 赤色を選ぶ", GetColor(255, 110, 110));
    DrawString(20, 140, "2 : 青色を選ぶ", GetColor(100, 180, 255));
    DrawString(20, 165, "3 : 緑色を選ぶ", GetColor(120, 255, 150));
    DrawString(20, 195, "Z長押し : 連続生成  X : 120体まで追加", GetColor(255, 235, 130));

    // 現在選択中の色を表示する。
    // この関数は前回のSlimeModel.hに定義している。
    const int color = slime.GetSelectedColor();
    const char* colorText = "選択中 : 赤";
    unsigned int textColor = GetColor(255, 110, 110);
    if (color == 2)
    {
        colorText = "選択中 : 青";
        textColor = GetColor(100, 180, 255);
    }
    else if (color == 3)
    {
        colorText = "選択中 : 緑";
        textColor = GetColor(120, 255, 150);
    }
    DrawString(20, 225, colorText, textColor);
    DrawString(20, 255, "子供スライムが出ないときは、床のある空いた場所へ移動してください。", GetColor(210, 210, 210));
    DrawString(20, 285, "表情 Y : a   U : b   I : c   O : d   P : e", GetColor(255, 190, 220));



    // 基底クラスの更新処理を呼びだす
    Scene::Draw();
}


void GameScene::Finalize()
{
	// BGM停止
	//Master::mpSoundManager->StopBGM();
}
