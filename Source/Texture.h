#pragma once
#include <string>
#include "DxLib.h"


class Texture
{
public:
	Texture(std::string filename, VECTOR position, int transFlag);  // コンストラクタ

	~Texture();  // デストラクタ

	void Draw();    // 描画
	void Update();  // 更新処理


	// セッター関数 // 
	// ポジション設定
	void SetPosition(VECTOR centerPosition) { mvPosition = centerPosition; }

	// ゲッター関数 //
	// ポジション取得
	VECTOR GetPosition() { return mvPosition; }

	// サイズ取得
	int GetSizeX() { return mnSizeX; }
	int GetSizeY() { return mnSizeY; }

	int GetHandle() const { return mnHandle;}
	// 半径サイズ取得
	float GetRadius() { return mfRadius; }


	// 画像サイズ指定付き描画
	void DrawSize(const VECTOR& pos, const VECTOR& size) const
	{
		DrawExtendGraph(static_cast<int>(pos.x), static_cast<int>(pos.y),
			static_cast<int>(pos.x + size.x), static_cast<int>(pos.y + size.y),
			mnHandle, mnTransFlag);
	}

private:
	
	int mnHandle;      // 読み込んだ画像のハンドル
	VECTOR mvPosition; // ポジション
	int mnSizeX;       // 画像の幅
	int mnSizeY;       // 画像の高さ
	int mnTransFlag;   // 画像の透過を有効にするか
	float mfRadius;    // 半径

};
