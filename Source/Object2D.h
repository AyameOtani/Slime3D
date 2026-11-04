#pragma once
#include <string>
#include "Object.h"	// 基底クラス

// クラスの前方宣言
class Texture;
class TextureAnimation;
class BlockMap;

// 
// 2D オブジェクトの基底クラス
// 2D オブジェクト（プレイヤーや敵など）を作る際は、
// 必ずこれを継承して作成する　
// 
class Object2D : public Object
{
public:    // enum, struct, 定数の定義


public:
	Object2D(std::string filename, VECTOR initPos);

	// コンストラクタ（アニメーション用）
	Object2D(VECTOR initPos, std::string filename, int allNum, int numX, int numY, int interval);
	
	// コンストラクタ(Player用)
	Object2D(const VECTOR initPos);

	virtual ~Object2D();
	virtual void Update() override;
	virtual void Draw()override;

public:   // ゲッター・セッター
	virtual float GetRadius(); // 半径の取得

	// プレイヤーで使うため
	int GetSizeX();  // 幅
	int GetSizeY();  // 高さ

	virtual ObjectDimension GetDimension()const override { return ObjectDimension::D2; } // 2Dオブジェクトであることを返す


protected:
	//2Dの要素は何が必要か考えて書く
	Texture* mpTexture;   // 画像
	TextureAnimation* mpTextureAnimation; // アニメーション画像
};