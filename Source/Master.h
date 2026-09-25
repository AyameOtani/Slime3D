#pragma once

#include "ObjectManager.h"
#include "GameManager.h"


//
// ゲーム内でひとつだけしか存在しないものを管理するクラス
// 例えば、
// SceneManager 　　だったり、（シーン管理クラス）
// SoundManager 　　だったり、（サウンド管理クラス）
// ResourceManager  だったり、（リソース（画像やモデル）管理クラス）
// を、どこからでも参照できるようにしておきたいので
// こういったクラスを作る
// 

class Master
{
public:

	static ObjectManager* mpObjectManager;
	static GameManager* mpGameManager; // ゲームマネージャーのポインタ

};
