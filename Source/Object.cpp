#include "Object.h"
#include "Master.h"         //今のシーンが誰なのか知りたい
#include "ObjectManager.h"
#include "Scene.h"

//*---------------------------------------------------------------------------------------
//*【?】コンストラクタ
//*----------------------------------------------------------------------------------------
Object::Object(const VECTOR& initPos)
	: mTransform({ initPos, VGet(0.0f, 0.0f, 1.0f), VGet(1.0f, 1.0f, 1.0f) })
	, mbDeleteFlag(false)
	, mbActiveFlag(true)
{
	// 現在シーンのオブジェクトマネージジャーに自身 (this) を追加する
	Master::mpGameManager->GetSceneManager()->GetCurrentScene()->GetObjectManager()->AddObject(this);
}

Object::Object()
	: mTransform({ VGet(0.0f,0.0f,0.0f), VGet(0.0f, 0.0f, 1.0f), VGet(1.0f, 1.0f, 1.0f) })
	, mbDeleteFlag(false)
{
	// 現在シーンのオブジェクトマネージジャーに自身 (this) を追加する
	Master::mpGameManager->GetSceneManager()->GetCurrentScene()->GetObjectManager()->AddObject(this);
}