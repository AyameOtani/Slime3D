#include "Player.h"
#include "Stage.h"
#include "Master.h"

using namespace Game::Slime;
using namespace Helper::DxLib_Math;
using namespace Helper::Math;

namespace
{
	constexpr float PLAYER_TEST_RADIUS				= 0.5f;		// プレイヤーのテスト用半径
	constexpr float PLAYER_TEST_HEIGHT				= 1.0f;		// プレイヤーのテスト用高さ
    constexpr float PLAYER_COLLISION_RADIUS         = 0.5f;     // プレイヤーの衝突半径
	constexpr float GROUNDED_FRICTION_COEFFICIENT	= 0.86f;	// 接地時の摩擦係数
	constexpr float AIRBORNE_FRICTION_COEFFICIENT	= 0.985;	// 空中時の摩擦係数
}

//*---------------------------------------------------------------------------------------
//*【?】コンストラクタ
//*----------------------------------------------------------------------------------------
Player::Player()
	: Object3D()
	, m_IsCameraRelativeMove(true)
	, m_IsGrounded(true)
	, m_MoveMaxSpeed(0.16f)
	, m_DeltaTime(0.0f)
	, m_Acceleration(60.0f)
	, m_Velocity(VEC_AllZero())
	, m_CameraForward(VEC_AllZero())
	, m_OldSpace(false)
{

}

//*---------------------------------------------------------------------------------------
//*【?】デストラクタ
//*----------------------------------------------------------------------------------------
Player::~Player()
{

}

//*---------------------------------------------------------------------------------------
//*【?】初期化
//*
//* [引数]
//* definition : スライムの固定データの参照
//* splitPhase : 分裂の状態 
//* 
//* [返値]なし
//*----------------------------------------------------------------------------------------
void Player::Init()
{
}

//*---------------------------------------------------------------------------------------
//*【?】更新処理
//*
//* [引数]なし
//* [返値]なし
//*----------------------------------------------------------------------------------------
void Player::Update()
{
	bool wasGrounded = m_IsGrounded;

	// 移動入力
	VECTOR input = VGet(0.0f, 0.0f, 0.0f);
	if (CheckHitKey(KEY_INPUT_W)) input.z += 1.0f;
	if (CheckHitKey(KEY_INPUT_S)) input.z -= 1.0f;
	if (CheckHitKey(KEY_INPUT_A)) input.x -= 1.0f;
	if (CheckHitKey(KEY_INPUT_D)) input.x += 1.0f;

	// 移動入力がない
	if (VSize(input) > 0.001f)
	{
		// 斜め入力は矢印が長くなるので、長さを1にそろえ、斜めだけ加速が強くなるのを防ぐ。
		input = VNorm(input);
	}

	// 移動の基準の前方向
	VECTOR forward = VGet(0.0f, 0.0f, 1.0f);

	//
	// カメラの向きに合わせる場合
	//
	if (m_IsCameraRelativeMove)
	{
		VECTOR cameraforward = m_CameraForward;

		// カメラの上下の向きは移動に使用しない
		cameraforward.y = 0.0f;

		// 正規化して前方向に
		if (VSquareSize(cameraforward) > 0.000001f)
		{
			forward = VNorm(cameraforward);
		}
	}

	// 右ベクトル
	const VECTOR right = VGet(forward.z, 0.0f, -forward.x);

	// 移動入力を合わせる
	VECTOR moveDirection = VAdd(
		VScale(forward, input.z),
		VScale(right, input.x)
	);

	if (VSquareSize(moveDirection) > 0.000001f)
	{
		moveDirection = VNorm(moveDirection);

		m_Velocity.x += moveDirection.x * m_Acceleration * m_DeltaTime;
		m_Velocity.z += moveDirection.z * m_Acceleration * m_DeltaTime;
	}

	// 摩擦力
	// 速度に1未満を掛けて減速。地上は止まりやすく、空中は勢いを残す設定。
	const float friction = m_IsGrounded ? 
		GROUNDED_FRICTION_COEFFICIENT : AIRBORNE_FRICTION_COEFFICIENT;

	// 減衰率
	const float damping = std::pow(friction, m_DeltaTime * 60.0f);
	m_Velocity.x *= damping;
	m_Velocity.z *= damping;

	// 最大速度
	float horizontalSpeed = sqrtf(
		m_Velocity.x * m_Velocity.x +
		m_Velocity.z * m_Velocity.z);

	if (horizontalSpeed > m_MoveMaxSpeed)
	{
		// 速度の方向を保ち、長さだけを上限にそろえるための倍率。
		float rate = m_MoveMaxSpeed / horizontalSpeed;

		m_Velocity.x *= rate;
		m_Velocity.z *= rate;
	}


	// ジャンプ
	bool space = CheckHitKey(KEY_INPUT_SPACE) != 0;

	// Spaceを押した瞬間、かつ前回地面にいた場合だけジャンプ。
	if (space && !m_OldSpace && wasGrounded)
	{
		m_Velocity.y = 0.25f;
	}

	m_OldSpace = space;

	// トランスフォームに反映
	mTransform.position = VAdd(mTransform.position, m_Velocity);

	m_IsGrounded = false;
	Stage* stage = Stage::Active();

	if (stage == nullptr || m_DeltaTime <= 0.0f)
	{
		return;
	}

	// SlimeModelの値を60fps基準で秒単位へ換算した仮値
	constexpr float GRAVITY = 1.0f;
	constexpr float MAX_FALL_SPEED = 0.3f;

	// 接地中も重力をかけ、床との接触を調べ続ける
	m_Velocity.y = (std::max)(
		m_Velocity.y - GRAVITY * m_DeltaTime,
		-MAX_FALL_SPEED
		);

	VECTOR position = mTransform.position;

	// 一度の移動距離が大きい場合は、小分けにして衝突を調べる
	const float travelDistance = VSize(m_Velocity) * m_DeltaTime;
	const float maxStepDistance = PLAYER_COLLISION_RADIUS * 0.5f;

	const int stepCount = (std::max)(
		1,
		static_cast<int>(
			std::ceil(travelDistance / maxStepDistance)
			)
		);

	const float stepTime = m_DeltaTime / static_cast<float>(1);

	for (int i = 0; i < 1; ++i)
	{
		position = VAdd(
			position,
			VScale(m_Velocity, stepTime)
		);

		// 今回の位置で接地しているか、改めて調べる
		m_IsGrounded = false;

		float impact = 0.0f;
		float wallDepth = 0.0f;
		VECTOR wall = VGet(0.0f, 0.0f, 0.0f);

		stage->ResolveSphere(
			position,
			m_Velocity,
			PLAYER_COLLISION_RADIUS,
			m_IsGrounded,
			impact,
			wall,
			wallDepth
		);

		// 
		// 着地エフェクト
		//
		if (!wasGrounded && m_IsGrounded && m_ImpactUponLandingEffectCoolDown <= 0.0f)
		{
			int handle = Master::mpEffectManager->PlayEffect3D("ImpactUponLanding_Player");
			Master::mpEffectManager->SetPosEffect3D(handle, position);
			Master::mpEffectManager->SetScaleEffect3D(handle, VEC_GetScalar(1.0f));

			// 連続でエフェクトが出ないようにクールダウンを設定
			m_ImpactUponLandingEffectCoolDown = 0.5f;	
		}
	}

	m_ImpactUponLandingEffectCoolDown =
		(std::max)(0.0f, m_ImpactUponLandingEffectCoolDown - m_DeltaTime);


	// 押し戻し後の座標を、本体へ反映する
	mTransform.position = position;
}

//*---------------------------------------------------------------------------------------
//*【?】描画処理
//*
//* [引数]なし
//* [返値]なし
//*----------------------------------------------------------------------------------------
void Player::Draw()
{
	VECTOR crntPos = mTransform.position;

	VECTOR pos1 = crntPos;
	VECTOR pos2 = pos1;
	pos2.y += PLAYER_TEST_HEIGHT;	// 高さ
	
	// プレイヤーを仮でカプセルで描画
	DrawCapsule3D(pos1, pos2, PLAYER_TEST_RADIUS, 32, GetColor(255, 64, 64), GetColor(255, 255, 255), true);
}


//*---------------------------------------------------------------------------------------
//*【?】移動に必要なパラメータの設定
//*
//* [引数]
//* camerafoward : カメラの向き
//* dt           : デルタタイム
//* 
//* [返値]なし
//*----------------------------------------------------------------------------------------
void Player::SetMoveParameters(const VECTOR& camerafoward, float dt)
{
	m_CameraForward = camerafoward;
	m_DeltaTime = dt;
}
