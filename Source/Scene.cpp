#include "Scene.h"
#include "ObjectManager.h"

Scene::Scene()
{
	// オブジェクトマネージャーの作成
	mpObjectManager = new ObjectManager();
}

Scene::~Scene()
{
	delete mpObjectManager;
}

// 更新
void Scene::Update()
{
	if (mpObjectManager != nullptr)
	{
		mpObjectManager->Update();
	}

}

// 描画
void Scene::Draw()
{

	if (mpObjectManager != nullptr)
	{
		mpObjectManager->Draw();
	}

}