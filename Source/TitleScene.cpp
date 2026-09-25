#include "TitleScene.h"
#include "Utility.h" // 呼び出すと、SCREEN_WIDTHとかを使える
#include "DxLib.h"
#include "Master.h"
#include "inputManager.h"
#include "SlimeModel.h"
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



TitleScene::TitleScene()
	: Scene()     // 基底クラスのコンストラクタを呼び出しておく
{
}

TitleScene::~TitleScene()
{

}

bool TitleScene::SlimeInitialize()
{
    // 形状変化（モーフィング）に使用する3つのモデルを読み込む
   // Resourceフォルダ内のMQOファイルを指定している
   // 戻り値をloadedに保存し、読み込みに成功したかを確認する
    bool loaded = slime.LoadMorphModels(
        "Resource\\Slime\\ibitu1.mqo",
        "Resource\\Slime\\ibitu2.mqo",
        "Resource\\Slime\\slime.mqo");

    // !loadedは「loadedがfalseの場合」という意味
    if (!loaded)
    {
        // 読み込みに失敗した場合、確認すべき内容を表示する
        // \nは、メッセージ内で改行するための記号
        // この部分には終了処理がないので、表示後も処理は続く
        MessageBox(nullptr,
            "MQOファイルの一部を読み込めませんでした。\n"
            "Resourceフォルダと、各モデルの頂点数713個を確認してください。",
            "MQO読み込みエラー", MB_OK);

        return false;
    }

    return true;
}

void TitleScene::Initialize()
{
    // スライムを初期状態にする
    // 具体的な初期化処理はSlimeModel側に書かれている
    slime.Initialize();

    SlimeInitialize();

    // ============================================
    // ステージの作成
    // ============================================

   

    // push_backは、一覧の末尾にデータを1個追加する命令
    // 各箱は「最小座標・最大座標・色」の順に指定している
    //
    // このプログラムでは、
    // X：横方向、Y：高さ方向、Z：奥行き方向として扱う

    // 地面
    // Xは-5.0～5.0、Yは-0.5～0.0、Zは-5.0～3.5
    // 上面の高さがY=0.0になる、薄くて広い箱
    boxes.push_back({ VGet(-5.0f,-0.5f,-5.0f), VGet(5.0f,0.0f,3.5f), GetColor(85,100,90) });

    // Zが正の方向にある壁
    // 高さは3.0、Z方向の厚さは0.4
    boxes.push_back({ VGet(-2.2f,0.0f,2.8f), VGet(2.2f,3.0f,3.2f),  GetColor(100,115,110) });

    // Xが負の側にある障害物
    // 地面からの高さは1.0
    boxes.push_back({ VGet(-3.8f,0.0f,-1.0f), VGet(-2.2f,1.0f,0.5f), GetColor(110,105,95) });

    // Xが正の側にある障害物
    // 地面からの高さは0.8
    boxes.push_back({ VGet(2.0f,0.0f,-2.8f), VGet(2.65f,0.8f,-1.0f), GetColor(115,100,85) });

}


void TitleScene::Update()
{
    // --------------------------------------------
     // スライムの状態を更新する
     // --------------------------------------------

      // ステージの箱の一覧を渡して、スライムの状態を更新する
      // 移動や衝突などの具体的な処理はSlimeModel::Update側にある
    slime.Update(boxes);

    // --------------------------------------------
    // カメラの位置と向きを決める
    // --------------------------------------------

    // 更新後のスライムの位置を取得する
    VECTOR p = slime.GetPosition();

    // カメラが注目する位置を作る
    // XとZはスライムの座標の20％、高さは常に1.0にする
    // スライムがX方向に1.0動くと、注目位置は0.2動く
    VECTOR target = VGet(p.x * 0.20f, 1.0f, p.z * 0.20f);

    // 注目位置からXに+7.5、Yに+5.5、Zに-10.0離れた場所に
    // カメラを置き、targetの方向を向ける
    // VAddは2つのVECTORを足し算する関数
    // UpVecYは、Y軸の正方向をカメラの上向きの基準にする指定
    SetCameraPositionAndTarget_UpVecY(VAdd(target, VGet(7.5f, 5.5f, -10.0f)), target);




	// 基底クラスの更新処理を呼びだす
	Scene::Update();
}


void TitleScene::Draw()
{
		Master::mpGameManager->GetFontManager()->DrawDotString(
			0,
			0,
			30,
			GetColor(255, 255, 255),
			"すらいむ"
		);



        // --------------------------------------------
     // 3D空間を描画する
     // --------------------------------------------

     // 地面・壁・障害物を描画する
        DrawWorld(boxes);

        // スライムを描画する
        // 描画処理にもステージの箱の一覧を渡している
        slime.Draw(boxes);

        // --------------------------------------------
        // 操作説明を画面に表示する
        // --------------------------------------------
        // DrawStringの最初の2つの値は、画面上の表示位置
        // 画面左上が(0, 0)で、右へ行くほどX、下へ行くほどYが増える
        // GetColorは、赤・緑・青を0～255で指定して色を作る

        // 移動とジャンプの操作説明
        DrawString(20, 20, "WASD : 移動 / SPACE : ジャンプ", GetColor(255, 255, 255));

        // モデル切り替えの操作説明
        // ここでは文字を表示しているだけで、キー入力の処理はしていない
        DrawString(20, 45, "1 : ibitu1.mqo", GetColor(255, 230, 100));
        DrawString(20, 70, "2 : ibitu2.mqo", GetColor(255, 230, 100));
        DrawString(20, 95, "3 : slime.mqo", GetColor(255, 230, 100));
        DrawString(20, 120, "4 : 原型", GetColor(180, 255, 210));




	// 基底クラスの更新処理を呼びだす
	Scene::Draw();
}


void TitleScene::Finalize()
{
	// BGM停止
	//Master::mpSoundManager->StopBGM();
}
