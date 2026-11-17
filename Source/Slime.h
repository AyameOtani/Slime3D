#pragma once
#include "Object3D.h"
#include "GameDefinitions.h"
#include "SlimeVisual.h"

namespace SlimeDef = Game::Slime;

// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:Slime Class --- */
//
// 【?】スライムの処理を行うクラス
//
// ***************************************************************************************
class Slime : public Object3D
{
private:
	SlimeDef::SlimeRuntime m_Runtime;						// ランタイムデータ
	const SlimeDef::SlimeSizeDefinition* m_pSizeDefinition; // サイズごとの固定定義データの参照
	SlimeVisual m_Visual;	
	bool m_ReturnRequested;									// プールに返却をリクエスト


	float m_Elapsed = 0.0f;

public:
	Slime();
	~Slime();

	void Init(const SlimeDef::SlimeSizeDefinition* definition, const SlimeDef::SLIME_SPLIT_PHASE splitPhase);
	void Update()override;
	void Draw()override;

	void ResetForReuse();										// 状態の初期化を行う（オブジェクトプール用）
	void RequestReturn();										// プールへの返却をリクエスト

	bool IsReturnRequested()const { return m_ReturnRequested; }	// プール返却リクエストフラグ
};

