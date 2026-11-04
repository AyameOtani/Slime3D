#pragma once

#include <list>
#include <vector>
#include "Object.h"


// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:ObjectManager Class --- */
//
// 【?】オブジェクトを管理するクラス
//		2Dや3Dのオブジェクトが作成されるとき（newされるとき）に必ず経由され
//		作成されたオブジェクトを一括管理するクラス
//		このクラスを作ることで、どのオブジェクトからでも同様なアクセスが可能になり、
//		より柔軟なゲーム制作がしやすくなる
// 
// 山本:すべてのオブジェクトの基底クラス「Object」を保持するようにしました。
//		2D、3Dのオブジェクトを区別するために、Objectクラスに「ObjectDimension」という列挙型を追加しています。
//		2D、3Dを区別して取得を行う場合は、ObjectDimensionを使って判定するようにしてください。
//		なるべく既存の処理から変更が少なくなるようにしていますが、必要に応じて修正してください。
//
// ***************************************************************************************
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
	// オブジェクトの追加
	void AddObject(Object* object);

	// 2Dオブジェクト全削除
	void DeleteAll2D();
	// 3Dオブジェクト全削除
	void DeleteAll3D();

	// 削除する必要のあるオブジェクトがあれば削除する
	// note: 全てのオブジェクトの更新が終わった後に呼び出す
	void DeleteAllIfNeeded();

	// 指定したタグのオブジェクトを取得
	// note: 該当するオブジェクトが複数ある場合、最初に見つけたオブジェクトを返す
	Object *GetObjectByTag(Object::Tag tag);

	// 指定下タグのオブジェクトのリストを取得
	// note: 該当するオブジェクトが複数ある場合、リスト化して全てのオブジェクトを返す
	std::vector<Object*> GetObjectListByTag(Object::Tag tag);


public:    // 3D系の関数宣言
	// 今は空っぽ

private:
	std::list<Object*> mObjectList;    // オブジェクトを管理するリスト
	


};