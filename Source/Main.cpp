#include "DxLib.h"
#include "Texture.h"
#include "TextureAnimation.h"
#include "Collision.h"
#include "SceneManager.h"
#include "Master.h"
#include "ObjectManager.h"
#include "FontManager.h"
#include "Scene.h"
#include "Utility.h"

/*
 @note リファレンス https://dxlib.xsrv.jp/dxfunc.html
*/

// Master クラスの静的メンバ変数定義
GameManager* Master::mpGameManager = nullptr;


/**
* @fn WinMain
* @brief Main関数
* @param[in] HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow
* @return int 0 正常終了／-1 エラー
* @details Main関数
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	// ウインドウモードで起動  falseにすると全画面
	ChangeWindowMode(true); 
	SetBackgroundColor(50, 50, 50);
	// サイズを合わせる
	SetGraphMode(Utility::SCREEN_WIDTH, Utility::SCREEN_HEIGHT, 0);  // モニター解像度に合わせる

	// DXライブラリ初期化
	if(DxLib_Init() == -1)
	{
		return -1;
	}

	// Manager関係の初期化
	//Master::mpGameManager->Initialize();
	Master::mpGameManager = new GameManager();

	// 描画先画面を裏画面に設定する
	SetDrawScreen(DX_SCREEN_BACK);


	// ============================================
	// 3D描画とカメラの基本設定
	 // ============================================
	// Zバッファ（奥行きの情報）を使う
	// 手前の物体が、奥の物体を隠すように描画できる
	SetUseZBuffer3D(TRUE);

	// 描画した物体の奥行きをZバッファに記録する
	SetWriteZBuffer3D(TRUE);

	// ライトによる明るさや陰影の計算を有効にする
	SetUseLighting(TRUE);

	// 遠くの物体が小さく見える「透視投影」に設定する
	// カメラの視野角は60度
	// この関数はラジアンで指定するため「度 × π ÷ 180」で変換する
	SetupCamera_Perspective(60.0f * DX_PI_F / 180.0f);

	// カメラに映す奥行きの範囲を設定する
	// カメラからの距離が0.1～100.0の範囲を描画対象にする
	SetCameraNearFar(0.1f, 100.0f);

	// ============================================
	// ライトの設定
	// ============================================
	// 一定の方向へ光を当てる「平行光源」に設定する
	// VGet(x, y, z)は、3つの値からVECTORを作る
	// ここでは光が進む方向を指定している
	ChangeLightTypeDir(VGet(-0.4f, -1.0f, 0.3f));

	// 環境光の色を設定する
	// 環境光は、直接ライトが当たらない部分にも与える基本的な明るさ
	// GetColorFは、赤・緑・青・アルファを通常0.0～1.0で指定する
	SetLightAmbColor(GetColorF(0.28f, 0.35f, 0.32f, 0.0f));

	// ============================================
	// 物体の表面の見え方（マテリアル）の設定
	// ============================================
	// 表面の色や光の反射の仕方をまとめる変数
	MATERIALPARAM material;

	// 拡散反射：ライトが当たったときの基本的な表面の色
	material.Diffuse = GetColorF(1, 1, 1, 1);

	// 環境光に対する反射色
	material.Ambient = GetColorF(0.30f, 0.40f, 0.35f, 0);

	// 鏡面反射：表面のツヤや、光が強く反射する部分の色
	material.Specular = GetColorF(1, 1, 1, 0);

	// 自己発光色：ライトとは別に、表面自身へ加える明るさや色
	// この値だけで、周囲の物体を照らすライトになるわけではない
	material.Emissive = GetColorF(0.03f, 0.08f, 0.05f, 0);

	// 光沢の鋭さ
	// 大きくするほど、光が反射して明るくなる範囲が狭くなる
	material.Power = 90.0f;

	// 作成したマテリアルの設定を適用する
	SetMaterialParam(material);

	// 頂点に設定された拡散反射色を使用する
	SetMaterialUseVertDifColor(TRUE);

	// 頂点に設定された鏡面反射色を使用する
	SetMaterialUseVertSpcColor(TRUE);




	// ゲームのメインループ
	// ProcessMessage() == 0  ウィンドウの☓ボタン押されていないかどうか
	// CheckHitKey(KEY_INPUT_ESCAPE) == 0  エスケープキーが押されていないかどうか
	int animationCounter = 0;
	int textureCurrentNum = 0;
	while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
	{
		//画面を初期化する
		ClearDrawScreen();

		int time = GetNowCount();

		// Managerクラスの更新
		Master::mpGameManager->Update();


		// SceneManagerの描画
		Master::mpGameManager->GetSceneManager()->Draw();


		// 裏画面の内容を表画面に映す
		ScreenFlip();

		// 17ミリ秒（秒間約60フレームだった場合の１フレーム当たりの経過時間）
		// 経過するまでここで待つ
		while (GetNowCount() - time < 17)
		{
			// 待つだけなので何も処理はしない
		}

		// 削除する必要のあるオブジェクトがあれば削除する
		Master::mpGameManager->GetSceneManager()
			->GetCurrentScene()
			->GetObjectManager()
			->DeleteAll2DIfNeeded();


		// ループする直前にシーン遷移チェックをいれておく
		Master::mpGameManager->GetSceneManager()->ChangeSceneIfNeeded();
	}

	// 終了処理
	// Manager関係の終了処理
	delete Master::mpGameManager;


	// DXライブラリ使用の終了
	DxLib_End();

	// ソフトの終了
	return 0;
}