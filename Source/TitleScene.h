#pragma once
#include "Scene.h" // シーン継承したいからインクルード
#include "HoverImageButton.h"


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
	// ホバーボタン関係 大谷
	HoverImageButton m_GameStart;
	HoverImageButton m_ExplainGraph;

	// ボタン位置の定数
	static constexpr int HOVER_BUTTON_X = 800; // ボタン位置X
	static constexpr int HOVER_BUTTON_Y = 600; // ボタン位置Y
	static constexpr int BUTTON_DIST_Y = 300;  // ボタン同士の縦の距離(一旦一列でやってます 大谷)

	// ---ホバーのサイズ ---
	// ボタンの通常時の拡大率を定義
	static constexpr float ButtonNormalScale = 0.4f;
	// ボタンのマウスホバー時の拡大率を定義
	static constexpr float ButtonHoverScale = 0.45f;
};
