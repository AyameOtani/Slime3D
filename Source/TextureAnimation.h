#pragma once
#include"DxLib.h"
#include <string>

class TextureAnimation
{
public:
	TextureAnimation(
		VECTOR position,
		std::string filename,
		int allNum,
		int xNum,
		int yNum,
		int interval
		);

	~TextureAnimation();


	void Update(); // 更新
	void Draw();   // 描画

	// セッター関数 // 
	// ポジション設定
	void SetPosition(VECTOR centerPosition) { mvPosition = centerPosition; }

	// ゲッター関数 //
	// ポジション取得
	VECTOR GetPosition() { return mvPosition; }


	// プレイヤーで使うため
	int GetSizeX() { return mnxNum; }  // 幅
	int GetSizeY() { return mnyNum; }  // 高さ

	//　半径取得
	float GetRadius() { return mfRadius; }


private:
	VECTOR mvPosition; // ポジション
	int mnCounter;     // アニメーションカウンタ
	int mnInterval;   // テクスチャ切り替えのフレーム数
	int mnCurrentNum;  // 何番目のテクスチャを表示するか
	int* mnHandleList; // 分割されたテクスチャのハンドルリスト
	int mnAllNum;      // テクスチャ分割数

	float mfRadius;    // 半径

	// メンバ変数の追加
	int mnxNum;
	int mnyNum;

};
