#pragma once
#include "Scene.h" // シーン継承したいからインクルード
#include "HoverImageButton.h"

// タイトルで表示するモデル関係のインクルード
#include "TitleSlimeModel.h"

// タイトル用カメラ（スライムを映すため）
#include "Camera.h"


// ホバーボタンのリソースパスを定義する名前空間
namespace  HoverImagePath
{
	static const std::string GameStartButton = "Resource/Button/game_start.png"; // ゲームスタートボタンの画像パス
	static const std::string ExplainButton = "Resource/Button/explain.png";		 // 操作説明ボタンの画像パス
}



class TitleScene : public Scene
{

public:

	TitleScene();
	virtual ~TitleScene();

	virtual void Initialize() override;
	virtual void Update() override;
	virtual void Draw() override;
	virtual void Finalize() override;

	// ホバーボタンの更新
	void UpdateHoverButton();


private:
	// タイトルで表示するオブジェクト関係
	TitleSlimeModel mTitleModel;

	// タイトル用カメラ（スライムの位置を映す）
	Camera mTitleCamera;


	// ホバーボタン関係 大谷
	HoverImageButton m_GameStart;
	HoverImageButton m_ExplainGraph;

	// タイトルのスライムの表示位置（奥行き）
	static constexpr float SLIME_POS_Z = 300.0f;

	// タイトルのカメラアングル（キー9で切り替わるアングルB：後ろ斜め上から）
	static constexpr Camera::CAMERA_ANGLE TITLE_CAMERA_ANGLE = Camera::CAMERA_ANGLE::ANGLE_B;

	// タイトルのスライムの表示位置を返す
	VECTOR GetSlimePosition() const;

	// ボタン位置の定数
	static constexpr int HOVER_BUTTON_X = Utility::SCREEN_WIDTH / 2; // ボタン位置X
	static constexpr int HOVER_BUTTON_Y = 700; // ボタン位置Y
	static constexpr int BUTTON_DIST_Y = 160;  // ボタン同士の縦の距離(一旦一列でやってます 大谷)

	// ---ホバーのサイズ ---
	// ボタンの通常時の拡大率を定義
	static constexpr float ButtonNormalScale = 0.3f;
	// ボタンのマウスホバー時の拡大率を定義
	static constexpr float ButtonHoverScale = 0.35f;
};
