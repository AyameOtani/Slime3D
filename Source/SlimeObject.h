#pragma once
#include "Object3D.h"
#include "GameDefinitions.h"
#include "SlimeVisual.h"

namespace SlimeDef = Game::Slime;

// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:SlimeObject Class --- */
//
// 【?】スライムの処理を行うクラス
//
// ***************************************************************************************
class SlimeObject : public Object3D
{
private:
	SlimeDef::SlimeRuntime m_Runtime;						// ランタイムデータ
	const SlimeDef::SlimeSizeDefinition* m_pSizeDefinition; // サイズごとの固定定義データの参照
	SlimeVisual m_Visual;	
	float m_PaintCooldown = 0.0f;
	bool m_HasPaintPoint = false;
	bool m_ReturnRequested;									// プールに返却をリクエスト
	VECTOR m_LastPaintPoint;	// 最後に塗った箇所

	//float elapsed = 0.0f;
public:
	SlimeObject();
	~SlimeObject();

	void Init(const SlimeDef::SlimeSizeDefinition* definition, const SlimeDef::SLIME_SPLIT_PHASE splitPhase, const SlimeDef::SLIME_TYPE type, int color);
	void Update()override;
	void Draw()override;
	void DrawShadow()override;

	void RefreshVisual();										// 状態の初期化を行う（オブジェクトプール用）
	void ResetForReuse();										// 状態の初期化を行う（オブジェクトプール用）
	void RequestReturn();										// プールへの返却をリクエスト

	void ApplySplit();	// 分裂する

	bool IsReturnRequested()const { return m_ReturnRequested; }	// プール返却リクエストフラグ

	void SetFace(int face);	// 表情の設定
	const SlimeDef::SlimeRuntime& GetRuntime() const { return m_Runtime; }	// ランタイムデータの取得
	float GetMoveSpeed() const {											// 移動速度の取得
		return m_pSizeDefinition ? m_pSizeDefinition->moveSpeed : 0.0f;
	}

private:
	bool UpdateSmallSuction(const Game::SuctionInput& suction, float deltaTime);
};

