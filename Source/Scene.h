#pragma once
//クラスの前方宣言
class ObjectManager;

//
// シーンの基底クラス
// タイトル、ゲーム、リザルド画面など、
// 画面を作る際は必ずこのクラスを継承する
//
//

class Scene
{
public:
	Scene();
	~Scene();
	
	// 純粋仮想関数

	// 初期化
	virtual void Initialize() = 0;
	// 更新
	virtual void Update();
	// 描画
	virtual void Draw();
	// 終了処理
	virtual void Finalize() = 0;

	// オブジェクトマネージャーの取得
	// ここでシーンのを取得出来るようにした。
	ObjectManager* GetObjectManager() { return mpObjectManager; }

private:
	ObjectManager* mpObjectManager;    // オブジェクト管理クラスのポインタ

};
