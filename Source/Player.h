#pragma once
#include "Object3D.h"
#include "GameDefinitions.h"
#include "SlimeModel.h"

// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:Player Class --- */
//
// 【?】プレイヤーの処理を行うクラス
//
// ***************************************************************************************
class Player :  public Object3D
{
private:
	bool m_IsCameraRelativeMove;	// カメラの向きに合わせて、移動方向を変えるかどうかのフラグ
	bool m_IsGrounded;				// 接地状態か
	float m_MoveMaxSpeed;			// 最大速度
	float m_DeltaTime;				// 前回の更新から経過した秒数
	float m_Acceleration;			// 加速度
	VECTOR m_Velocity;				// 移動速度
	VECTOR m_CameraForward;			// カメラの向き
	bool m_OldSpace;
	float m_ImpactUponLandingEffectCoolDown;
public:
	Player();
	~Player();

	void Init();
	void Update()override;
	void Draw()override;

	void SetIsCameraRelativeMove(bool flag) { m_IsCameraRelativeMove = flag; }	// カメラの向きに合わせて移動方向を変えるかのフラグ設定

	// 移動に必要なパラメータの設定
	void SetMoveParameters(const VECTOR& camerafoward, float dt);

private:
	void ResolveCollision(const BoxCollider& box);
	VECTOR ClosestPoint(const VECTOR& p, const BoxCollider& box);
};

