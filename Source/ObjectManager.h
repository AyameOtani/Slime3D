#pragma once

#include <list>
#include <vector>
#include "Object2D.h"

//
// オブジェクトを管理するクラス
// 2Dや3Dのオブジェクトが作成されるとき（newされるとき）に必ず経由され
// 作成されたオブジェクトを一括管理するクラス
// このクラスを作ることで、どのオブジェクトからでも同様なアクセスが可能になり、
// より柔軟なゲーム制作がしやすくなる
// 
class ObjectManager
{
public:
	ObjectManager();
	~ObjectManager();


	// 更新
	void Update();

	// 描画
	void Draw();


public:        // 2D系の関数宣言
	// 2Dオブジェクトの追加
	void AddObject(Object2D* object2D);

	// 2Dオブジェクト全削除
	void DeleteAll2D();

	// 削除する必要のあるオブジェクトがあれば削除する
	// note: 全てのオブジェクトの更新が終わった後に呼び出す
	void DeleteAll2DIfNeeded();

	// 指定したタグの２Dオブジェクトを取得
	// note: 該当するオブジェクトが複数ある場合、最初に見つけたオブジェクトを返す
	Object2D *GetObject2DByTag(Object2D::Tag tag);


	// 指定下タグの２Dオブジェクトのリストを取得
	// note: 該当するオブジェクトが複数ある場合、リスト化して全てのオブジェクトを返す
	std::vector<Object2D*> GetObject2DListByTag(Object2D::Tag tag);


public:    // 3D系の関数宣言
	// 今は空っぽ

private:
	std::list<Object2D*> mObject2DList;    // 2Dオブジェクトを管理するリスト
	


};