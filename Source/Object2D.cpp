#include "Object2D.h"
#include "Texture.h"
#include "Master.h"         //今のシーンが誰なのか知りたい
#include "ObjectManager.h"
#include "Scene.h"
#include "TextureAnimation.h"


// コンストラクタ
Object2D::Object2D(std::string filename, VECTOR initPos)
	: mvPosition(initPos)
	, mbDeleteFlag(false)
	, mpTextureAnimation(nullptr)
{
	// 現在シーンのオブジェクトマネージジャーに自身 (this) を追加する
	Master::mpGameManager->GetSceneManager()->GetCurrentScene()->GetObjectManager()->AddObject(this);

	// 画像生成
	mpTexture = new Texture(filename, initPos, true);
}




// コンストラクタ（アニメーション用）
Object2D::Object2D(VECTOR initPos, std::string filename, int allNum, int numX, int numY, int interval)
	: mvPosition(initPos)
	,mbDeleteFlag(false)
	,mpTexture(nullptr)  // テクスチャーアニメーションの場合は画像一味表示の法は使用しない
{
	// 現在のシーンの ObjectManager に自身（）this を追加
	Master::mpGameManager->GetSceneManager()->GetCurrentScene()->GetObjectManager()->AddObject(this);

	// テクスチャアニメーション作成
	mpTextureAnimation = new TextureAnimation(initPos, filename, allNum, numX, numY, interval);
}


// コンストラクタ（アニメーション用）
Object2D::Object2D(const VECTOR initPos)
	: mvPosition(initPos)
	,mbDeleteFlag(false)
	,mpTexture(nullptr)  // テクスチャーアニメーションの場合は画像一味表示の法は使用しない
{
	// 現在のシーンの ObjectManager に自身（）this を追加
	Master::mpGameManager->GetSceneManager()->GetCurrentScene()->GetObjectManager()->AddObject(this);
}

// デストラクタ
Object2D::~Object2D()
{

	// 画像破棄
	if (mpTexture != nullptr)
	{
		delete mpTexture;
	}

	// アニメーション画像破棄
	if (mpTextureAnimation != nullptr)
	{
		delete mpTextureAnimation;
	}
}

// 更新
void Object2D::Update()
{
	if (mpTexture != nullptr)
	{
		mpTexture->Update();
		
		// 座標設定
	     mpTexture->SetPosition(mvPosition);
	}

	// テクスチャアニメーションの更新
	if (mpTextureAnimation != nullptr)
	{
		mpTextureAnimation->Update();
		
		// 座標設定
		// こっちでもプレイヤーの奴をやっているから
		// プレイヤーのUp0dateに書かなくてもok
		mpTextureAnimation->SetPosition(mvPosition);
	}
	
}

// 描画
void Object2D::Draw()
{
	if (mpTexture != nullptr)
	{
		mpTexture->Draw();
	}

	// テクスチャアニメーション
	if (mpTextureAnimation != nullptr)
	{
		mpTextureAnimation->Draw();
	}
}

// 半径取得
float Object2D::GetRadius()
{
	if (mpTexture != nullptr)
	{
     return mpTexture->GetRadius();
	}

	 // テクスチャアニメーション
	 if (mpTextureAnimation != nullptr)
	 {
         return mpTextureAnimation->GetRadius();
	 }

	return 0.0f; // 上二つが生成されていない場合
}


// サイズX
int  Object2D::GetSizeX()
{
	if (mpTexture != nullptr)
	{
       return mpTexture->GetSizeX();
	}

	 // テクスチャアニメーション
	 if (mpTextureAnimation != nullptr)
	 {
         return mpTextureAnimation->GetSizeX();
	 }
	
	return 0; // 上二つが生成されていない場合
}

// サイズY
int  Object2D::GetSizeY()
{
	if (mpTexture != nullptr)
	{
       return mpTexture->GetSizeY();
	}

	 // テクスチャアニメーション
	 if (mpTextureAnimation != nullptr)
	 {
         return mpTextureAnimation->GetSizeY();
	 }
	
	return 0; // 上二つが生成されていない場合
}