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
public:

public:
	Object3D();
	Object3D(const VECTOR initPos);

	virtual ~Object3D();
	virtual void Update() override;
	virtual void Draw()override;

public:
	virtual ObjectDimension GetDimension()const override { return ObjectDimension::D3; } // 3Dオブジェクトであることを返す
};

