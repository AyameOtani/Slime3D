#pragma once
#include "Dxlib.h"

//////////////////////////////////////////////////////////////////////////////////////////
//
//		ゲーム用データ定義名前空間
// 
//////////////////////////////////////////////////////////////////////////////////////////
namespace Game
{
	// ---------------------------------------------------------------------------------------
	// 
	//		スライム用定義
	// 
	// ---------------------------------------------------------------------------------------
	namespace Slime
	{
		// ================================
		// 列挙体
		// ================================

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

		// ================================
		// 構造体
		// ================================

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
				sizes[static_cast<unsigned>(SLIME_SPLIT_PHASE::NUM)];	
		};

		/// @brief 実行時に変化するデータ
		struct SlimeRuntime
		{
			VECTOR velocity;					// 移動速度
			float radius = 0.0f;				// 現在の半径
			float mergeCooldown = 0.0f;			// 合体できるまでの残り時間
			bool isGrounded = false;			// 接地状態
			
			SLIME_TYPE type						// スライムの種類
				= SLIME_TYPE::NORMAL;			
			
			SLIME_SPLIT_PHASE splitPhase		// 現在の分裂段階
				= SLIME_SPLIT_PHASE::PHASE_0;	
		};


		/// @brief スライムを出現させるためのパラメータ
		struct SlimeSpawnParams
		{
			Slime::SLIME_TYPE type = Slime::SLIME_TYPE::NORMAL;	// スライムの種類
			Slime::SLIME_SPLIT_PHASE splitPhase					// 分裂状態
				= Slime::SLIME_SPLIT_PHASE::PHASE_0;

			VECTOR position = VGet(0.0f, 0.0f, 0.0f);			// 位置
			VECTOR direction = VGet(0.0f, 0.0f, 1.0f);			// 向き
		};
	}
}