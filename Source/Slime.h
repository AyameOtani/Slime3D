#pragma once
#include "Object3D.h"


// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:Slime Class --- */
//
// 【?】スライムの処理を行うクラス
//
// ***************************************************************************************
class Slime : public Object3D
{
public:
	// ============================================================
	// 列挙体
	// ============================================================

	/// @brief スライムの種類
	/// ※ 移動が早いやつとか、吸い込みにくいやつとか、種類を増やしたい場合
	enum class SLIME_TYPE
	{
		NORMAL,

		NUM,
	};

	/// @brief 分裂の段階
	enum class SLIME_SPLIT_PHASE
	{
		PHASE_0,	// 分裂していない状態
		PHASE_1,	// １段階目
		PHASE_2,	// ２段階目
		PHASE_3,	// ３段階目

		NUM,
	};

	// ============================================================
	// 構造体
	// ============================================================

	/// @brief スライムのサイズごとのデータ
	struct SlimeSizeDefinition
	{
		float radius = 0.0f;
		float moveSpeed = 0.0f;
		float mergeDelay = 0.0f;
	};

	/// @brief スライムの固定定義データ
	struct SlimeDefinition
	{
		// スライムのサイズ分持つ
		SlimeSizeDefinition 
			sizes[UINT_CAST(SLIME_SPLIT_PHASE::NUM)];	
	};

	/// @brief 実行時に変化するデータ
	struct SlimeRuntime
	{
		float speed = 0.0f;					// 現在の速度
		float size = 0.0f;					// 現在のサイズ
		SLIME_SPLIT_PHASE splitPhase		// 現在の分裂段階
			= SLIME_SPLIT_PHASE::PHASE_0;	
	};
private:
	SlimeRuntime m_Runtime;						  // ランタイムデータ
	const SlimeSizeDefinition* m_pSizeDefinition; // サイズごとの固定定義データの参照

public:
	Slime();
	~Slime();

	void Update()override;
	void Draw()override;

	void ResetForReuse();	// 状態の初期化を行う（オブジェクトプール用）
};

