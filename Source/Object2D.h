#pragma once
#include "DxLib.h"
#include <string>

// クラスの前方宣言
class Texture;
class TextureAnimation;
class BlockMap;

// 
// 2D オブジェクトの基底クラス
// 2D オブジェクト（プレイヤーや敵など）を作る際は、
// 必ずこれを継承して作成する　
// 
class Object2D
{
public:    // enum, struct, 定数の定義

	// オブジェクトを見分けるためのタグ
	// Object2D だけだと何か分からないあの出見分けるためのタグ
	enum Tag
	{
		// タグの番号被ったらバグるから要注意 
		PlayerSlime = 1000,  // プレイヤー
		Enemy3D = 1001,         // 敵

		// 増やせる
	};


public:
	Object2D(std::string filename, VECTOR initPos);

	// コンストラクタ（アニメーション用）
	Object2D(VECTOR initPos, std::string filename, int allNum, int numX, int numY, int interval);
	
	
	// コンストラクタ(Player用)
	Object2D(const VECTOR initPos);


	virtual ~Object2D();
	virtual void Update();
	virtual void Draw();

public:   // ゲッター・セッター
	void SetPosition(VECTOR pos) { mvPosition = pos; }  // 座標設定
	VECTOR GetPosition() { return mvPosition; }         // 座標取得

	void SetDeleteFlag(bool flag) { mbDeleteFlag = flag; }  // 削除フラグ設定
	bool IsDeleteFlag() { return mbDeleteFlag; }           // 削除フラグ取得

	void SetTag(Tag tag) { mnTag = tag; }  // タグ設定
	Tag GetTag() { return mnTag; }         // タグ取得


	virtual float GetRadius(); // 半径の取得

	// プレイヤーで使うため
	int GetSizeX();  // 幅
	int GetSizeY();  // 高さ


protected:
	//2Dの要素は何が必要か考えて書く
	Texture* mpTexture;   // 画像
	VECTOR mvPosition;    // 座標
	TextureAnimation* mpTextureAnimation; // アニメーション画像
	
private:
	bool mbDeleteFlag;    // 削除フラグ(これがtrue になっていると自動的に削除される（ように作る）)
	Tag mnTag;            // オブジェクトを見分ける用のタグ
	// プレイヤー用
	VECTOR mvDirection;
};