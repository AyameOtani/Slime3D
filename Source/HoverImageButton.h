#pragma once
#include "DxLib.h"
#include <string>

// マウスが乗ったら拡大する画像のクラス
// 呼び出して使えるようにするために作成しました 大谷

class HoverImageButton
{
public:

	HoverImageButton(
		float x,			 // Xの位置 中心基準
		float y,			 // Yの位置 中心基準
		float angle,		 // 角度
		std::string filename,// 画像ハンドル
		float rate,	         // 拡大率
		float changerate     // 変えた後の拡大率
	);

	~HoverImageButton();

	void Draw(); //描画
	void Update(); // 判定
	bool IsClicked(); // マウスが乗っててかつ押されたらの返すやつ
	bool GetIsHover() { return isHover; } // マウスが乗っているかを取得

	// ターンエンドボタンが押せる状態か プレイヤーのターンかのやつ
	void SetActive(bool active) { isActive = active; }


	//　アクセサ
	void SetPosition(float x, float y);
	void SetAngle(float angle);

private:
	// 定数定義
	static constexpr float CenterDivisor = 2.0f;                       // 中心位置を計算するための除算値
	static constexpr int InvalidGraphHandle = -1;                      // 画像の読み込み失敗や無効状態を表すハンドル値
	static constexpr int BlendModeParamMax = 255;                      // ブレンドモードの最大値（不透明度など）

	float mx;
	float my;
	float mAngle;
	int mnHandle;
	float mRate;
	float mChangeRate;

	bool isHover;
	bool isActive;
};