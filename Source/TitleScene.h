#pragma once
#include "Scene.h" // シーン継承したいからインクルード
#include "SlimeModel.h"
#include "Stage.h"
#include "Camera.h"

class TitleScene : public Scene
{

public:

	TitleScene();

	virtual ~TitleScene();

	// スライムのモデルが読み込まれたか
	bool SlimeInitialize();

	// 初期化
	virtual void Initialize() override;
	// 更新
	virtual void Update() override;
	// 描画
	virtual void Draw() override;
	// 終了処理
	virtual void Finalize() override;

private:
	// ============================================
	 // スライムの作成とモデルの読み込み
	 // ============================================
	// スライムを管理するオブジェクトを作る
	SlimeModel slime;

	// ステージ  山本
	Stage m_Stage;
	// カメラ  山本
	Camera m_Camera;

	// 地面や壁などに使う箱の一覧
   // 描画に加え、スライム側の処理にもこの一覧を渡す
	std::vector<BoxCollider> boxes;


};
