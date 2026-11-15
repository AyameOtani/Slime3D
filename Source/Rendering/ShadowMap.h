#pragma once
#include "DxLib.h"

// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:ShadowMap Class --- */
//
// 【?】シャドウの作成、描画などの管理を行う
//
// ***************************************************************************************
class ShadowMap
{
private:
	int m_ShadowMapHandle;	// シャドウマップのハンドル
	float m_SizeX;			// シャドウマップの横幅
	float m_SizeY;			// シャドウマップの縦幅

public:
	ShadowMap(float width, float height);
	~ShadowMap();

	bool Setup();				// 初期化
	void Finalize();			// 終了
	void ShadowMapWriteBegin();	// シャドウマップへの書き込み - 開始
	void ShadowMapWriteEnd();	// シャドウマップへの書き込み - 終了
	void ShadowMapBind();		// シャドウマップをバインドする
	void ShadowMapUnbind();		// シャドウマップのバインドを解く
	void TestDraw(int x1, int y1, int x2, int y2);

	void SetLightDirection(const VECTOR& direction);	// 影を落とす方向を設定 ※ 必ずSetup後に呼んでください
	void SetAdjustDepth(float depth);
	void SetDrawArea(const VECTOR& minPosition, const VECTOR& maxPosition);
	void ResetDrawArea();
};

