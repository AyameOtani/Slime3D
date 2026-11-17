#pragma once
#include "Object.h"

// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:Object3D Class --- */
//
// 【?】3Dオブジェクトの基底クラス
//
// ***************************************************************************************
class Object3D : public Object
{
protected:
	bool m_IsWriteShadow;	// シャドウを書き込むかどうか

public:
	Object3D();
	Object3D(const VECTOR initPos);

	virtual ~Object3D();
	virtual void Update() override;
	virtual void Draw()override;

	virtual void DrawShadow() {}	// 影を落とす場合は派生クラスで実装

	void SetIsWriteShadow(bool flag) { m_IsWriteShadow = flag; }	// シャドウを書き込むかどうかのフラグ設定
	bool GetIsWriteShadow()const { return m_IsWriteShadow; }		// シャドウを書き込むかどうかのフラグ取得

public:
	virtual ObjectDimension GetDimension()const override { return ObjectDimension::D3; } // 3Dオブジェクトであることを返す
};

