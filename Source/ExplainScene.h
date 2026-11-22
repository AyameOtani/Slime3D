#pragma once
#include "Scene.h" // シーン継承したいからインクルード
#include "HoverImageButton.h"


// 画面遷移の貯めに仮で作成したシーンです。
// 将来的には説明画像や操作の画像を表示するシーンにしても良いと思います 大谷
class ExplainScene : public Scene
{

public:

	ExplainScene();
	virtual ~ExplainScene();

	virtual void Initialize() override;
	virtual void Update() override;
	virtual void Draw() override;
	virtual void Finalize() override;
};
