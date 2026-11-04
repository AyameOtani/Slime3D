#pragma once
#include "DxLib.h"

// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:Object Class --- */
//
//  ★★★抽象クラス★★★
//
// 【?】すべてのオブジェクトの基底クラス
//		このクラスを継承して、2Dオブジェクトや3Dオブジェクトを作る
//
// ***************************************************************************************
class Object 
{
public:    
	//*****************************************
	// enum, struct, 定数の定義
	//*****************************************

	// オブジェクトを見分けるためのタグ
	// Object2D だけだと何か分からないあの出見分けるためのタグ
	enum Tag
	{
		// タグの番号被ったらバグるから要注意 
		PlayerSlime = 1000,  // プレイヤー
		Enemy3D = 1001,         // 敵

		// 増やせる
	};

	// 座標、向き、スケールの情報をまとめたもの
	struct ObjectTransform
	{
		VECTOR position;   // 座標
		VECTOR direction;  // 向き
		VECTOR scale;	   // 拡縮
	};

	// オブジェクトの次元を表す列挙型
	// 2Dオブジェクトか3Dオブジェクトかを区別するために使用する
	enum class ObjectDimension 
	{ 
		D2, D3 
	};

protected:
	//*****************************************
	// メンバ変数
	//*****************************************
	bool mbDeleteFlag;			// 削除フラグ(これがtrue になっていると自動的に削除される（ように作る）)
	Tag mnTag;					// オブジェクトを見分ける用のタグ
	ObjectTransform mTransform;	// 座標、向き、スケールの情報をまとめたもの

public:
	// コンストラクタ(Player用)
	Object();
	Object(const VECTOR& initPos);
	virtual ~Object() = default;
	virtual void Update() = 0;	// 更新処理
	virtual void Draw() = 0;	// 描画処理

public:  
	//*****************************************
	// ゲッター・セッター
	//*****************************************
	void SetPosition(const VECTOR& pos) { mTransform.position = pos; }		// 座標設定
	VECTOR GetPosition()const { return mTransform.position; }				// 座標取得
	
	void SetDirection(const VECTOR& dir) { mTransform.direction = dir; }	// 向き設定
	VECTOR GetDirection()const { return mTransform.direction; }				// 向き取得

	void SetScale(const VECTOR& scl) { mTransform.scale = scl; }			// 拡縮設定
	VECTOR GetScale() const { return mTransform.scale; }					// 拡縮取得

	void SetTransform(const ObjectTransform& transform) { mTransform = transform; }	// トランスフォームの設定
	ObjectTransform GetTransform() const { return mTransform; }						// トランスフォームの取得

	void SetDeleteFlag(bool flag) { mbDeleteFlag = flag; }  // 削除フラグ設定
	bool IsDeleteFlag() const { return mbDeleteFlag; }      // 削除フラグ取得

	void SetTag(Tag tag) { mnTag = tag; }  // タグ設定
	Tag GetTag() const{ return mnTag; }    // タグ取得

	virtual ObjectDimension GetDimension() const = 0;	// 派生先でオブジェクトの次元を返すようにする
};

