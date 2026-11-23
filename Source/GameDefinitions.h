#pragma once
#include "Dxlib.h"

//////////////////////////////////////////////////////////////////////////////////////////
//
//		ゲーム用データ定義名前空間
// 
//////////////////////////////////////////////////////////////////////////////////////////
namespace Game
{
	static constexpr float SHADOW_SIZE_X = 4096.0f;
	static constexpr float SHADOW_SIZE_Y = 4096.0f;


	struct BoxCollider
	{
		VECTOR minPos;
		VECTOR maxPos;
		unsigned int color;
	};

	/// @brief 塗る色
	enum class PAINT_COLOR { RED, BLUE, GREEN, RAINBOW };

	/// @brief 衝突した面データ
	struct SurfaceHit
	{
		int triangleId;
		VECTOR point;	
		VECTOR normal;
	};

	/// @brief 塗る範囲
	struct PaintCoverage
	{
		double dirtyArea;		// 塗った範囲
		double paintableArea;	// 塗ることが可能な範囲
	};


	/// @brief 吸い込み口用
	struct SuctionInput
	{
		bool active = false;

		VECTOR nozzle					// ノズル先端のワールド座標
			= VGet(0.0f, 0.0f, 0.0f);

		VECTOR forward					// 吸引方向
			= VGet(0.0f, 0.0f, 0.0f);
	};


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
			NORMAL,		// 通常
			JUMP,		// ジャンプするやつ
			SHOOTING,	// ペイント弾を撃ってくるやつ
			NUM
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

		/// @brief スライムのステート
		enum class SlimeState
		{
			Free,       // 自分で移動・ジャンプ・発射できる
			Sucked,     // 掃除機に吸われている
			Launched,   // 掃除機から射出された
			Stunned,	// 気絶状態
			Captured,	// 回収された 
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
			VECTOR velocity					// 移動速度
				= VGet(0.0f, 1.0f, 0.0f);
			VECTOR groundNormal				// 地面の法線
				= VGet(0.0f, 1.0f, 0.0f);
			float radius = 0.0f;			// 現在の半径
			float mergeCooldown = 0.0f;		// 合体できるまでの残り時間
			float actionElapsed = 0.0f;		// この個体が次にジャンプ・発射などを行うまでの経過時間
			bool isGrounded = false;		// 接地状態
			bool isMoveDir = false;			// 移動方向を決定したか
			int color = 1;					// 1=赤、2=青、3=緑
			
			SlimeState state				// 状態
				= SlimeState::Free;

			SLIME_TYPE type					// スライムの種類
				= SLIME_TYPE::NORMAL;		
			
			SLIME_SPLIT_PHASE splitPhase	// 現在の分裂段階
				= SLIME_SPLIT_PHASE::PHASE_0;	
		};


		/// @brief スライムを出現させるためのパラメータ
		struct SlimeSpawnParams
		{
			Slime::SLIME_TYPE type = Slime::SLIME_TYPE::NORMAL;	// スライムの種類
			Slime::SLIME_SPLIT_PHASE splitPhase					// 分裂状態
				= Slime::SLIME_SPLIT_PHASE::PHASE_0;
			int color = 1;										// 1=赤、2=青、3=緑

			VECTOR position = VGet(0.0f, 0.0f, 0.0f);			// 位置
			VECTOR direction = VGet(0.0f, 0.0f, 1.0f);			// 向き
		};
	}
}