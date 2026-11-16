#include "SlimeObject.h"
#include "Master.h"
#include "Stage.h"

using namespace Game::Slime;
using namespace Helper::DxLib_Math;
using namespace Helper;


namespace
{
	constexpr float GROUNDED_FRICTION_COEFFICIENT = 0.86f;	// 接地時の摩擦係数
	constexpr float AIRBORNE_FRICTION_COEFFICIENT = 0.985;	// 空中時の摩擦係数
	constexpr float GRAVITY = 30.0f;
	constexpr float MAX_FALL_SPEED = 10.0f;
	constexpr float JUMP_POWER = 10.0f;
}

//*---------------------------------------------------------------------------------------
//*【?】コンストラクタ
//*----------------------------------------------------------------------------------------
SlimeObject::SlimeObject()
	: Object3D()
	, m_Runtime()
	, m_pSizeDefinition(nullptr)
	, m_ReturnRequested(false)
{

}

//*---------------------------------------------------------------------------------------
//*【?】デストラクタ
//*----------------------------------------------------------------------------------------
SlimeObject::~SlimeObject()
{

}

//*---------------------------------------------------------------------------------------
//*【?】スライムの初期化
//*
//* [引数]
//* definition : スライムの固定データの参照
//* splitPhase : 分裂の状態 
//* type       : スライムの種別 
//* color      : 色 1=赤、2=青、3=緑
//* 
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeObject::Init(const SlimeSizeDefinition* definition, const SLIME_SPLIT_PHASE splitPhase, const SlimeDef::SLIME_TYPE type, int color)
{
	m_pSizeDefinition = definition;
	m_Runtime.type = type;
	m_Runtime.splitPhase = splitPhase;
	m_Runtime.radius = m_pSizeDefinition->radius;
	m_Runtime.color = color;

	// 見た目の初期化
	m_Visual.Setup();
	m_Visual.Reset();

}

//*---------------------------------------------------------------------------------------
//*【?】更新処理
//*
//* [引数]なし
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeObject::Update()
{
	float deltaTime = Master::mpGameManager->GetTimeManager()->get_DeltaTime();
	bool wasGrounded = m_Runtime.isGrounded;

	// 移動方向の決定
	if (!m_Runtime.isMoveDir)
	{
		m_Runtime.isMoveDir = true;

		mTransform.direction = VGet(
			Math::RandRange(-1.0f, 1.0f),
			0.0f,
			Math::RandRange(-1.0f, 1.0f)
		);
	}
	
	VECTOR moveDir = mTransform.direction;
	const float moveSpeed = m_pSizeDefinition->moveSpeed;
	moveDir.y = 0.0f;	// 水平成分の移動にする
	if (VSquareSize(moveDir) > 0.000001f)
	{
		moveDir = VNorm(moveDir);
	}

	VECTOR velocity = VEC_AllZero();
	velocity.x = moveDir.x * moveSpeed;
	velocity.z = moveDir.z * moveSpeed;
	velocity.y = m_Runtime.velocity.y;

	// 有効チェック
	Stage* stage = Stage::Active();
	if (stage == nullptr || deltaTime <= 0.0f)
	{
		return;
	}

	m_Runtime.isGrounded = false;

	// 接地中も重力をかけ、床との接触を調べ続ける
	velocity.y = (std::max)(
		velocity.y - GRAVITY * deltaTime,
		-MAX_FALL_SPEED
		);
	VECTOR position = mTransform.position;

	// 一度の移動距離が大きい場合は、小分けにして衝突を調べる
	const float travelDistance = VSize(velocity) * deltaTime;
	const float maxStepDistance = m_Runtime.radius * 0.5f;

	const int stepCount = (std::max)(
		1,
		static_cast<int>(
			std::ceil(travelDistance / maxStepDistance)
			)
		);

	const float stepTime = deltaTime / static_cast<float>(stepCount);

	for (int i = 0; i < stepCount; ++i)
	{
		position = VAdd(
			position,
			VScale(velocity, stepTime)
		);

		// 今回の位置で接地しているか、改めて調べる
		m_Runtime.isGrounded = false;

		float impact = 0.0f;
		float wallDepth = 0.0f;
		VECTOR wall = VGet(0.0f, 0.0f, 0.0f);

		stage->ResolveSphere(
			position,
			velocity,
			m_Runtime.radius,
			m_Runtime.isGrounded,
			impact,
			wall,
			wallDepth
		);
	}

	m_Runtime.velocity = velocity;

	if (position.y <= -30.0f)
	{
		position = VGet(0.0f, 30.0f, 0.0f);
	}

	// 押し戻し後の座標を、本体へ反映する
	mTransform.position = position;

	m_Runtime.velocity = velocity;
	mTransform.position = position;

	// 接地中に、見た目へ渡す床の法線を取得する。
	if (m_Runtime.isGrounded)
	{
		const float radius = m_Runtime.radius;
		constexpr float FLOOR_MARGIN = 0.02f;

		float floorY = 0.0f;
		VECTOR floorNormal = VGet(0.0f, 1.0f, 0.0f);

		const bool found = stage->FindFloor(
			position.x,
			position.z,
			position.y - radius * 2.0f - FLOOR_MARGIN,
			position.y - radius + FLOOR_MARGIN,
			floorY,
			&floorNormal);

		if (found)
		{
			m_Runtime.groundNormal = VNorm(floorNormal);
		}
	}

	// 見た目更新
	m_Visual.Update(*this);

	
	if (elapsed >= 3.0f)
	{
		int handle = Master::mpEffectManager->PlayEffect3D("SlimeExplosion");
		Master::mpEffectManager->SetPosEffect3D(handle, position);

		m_ReturnRequested = true;

		SlimeDef::SlimeSpawnParams spawn;
		spawn.position = position;
		spawn.type = SlimeDef::SLIME_TYPE::NORMAL;
		
		SLIME_SPLIT_PHASE newPhase = m_Runtime.splitPhase;
		if (newPhase == SLIME_SPLIT_PHASE::PHASE_0)newPhase = SLIME_SPLIT_PHASE::PHASE_1;
		else if (newPhase == SLIME_SPLIT_PHASE::PHASE_1)newPhase = SLIME_SPLIT_PHASE::PHASE_2;
		else if (newPhase == SLIME_SPLIT_PHASE::PHASE_2)newPhase = SLIME_SPLIT_PHASE::PHASE_3;
		else if (newPhase == SLIME_SPLIT_PHASE::PHASE_3)newPhase = SLIME_SPLIT_PHASE::PHASE_0;

		spawn.splitPhase = newPhase;
		spawn.color = INT_CAST(Helper::Math::RandRange(1, 4));
		Master::mpGameManager->GetSlimeManager()->SpawnSlime(spawn);

		elapsed = 0.0f;
		return;
	}
	elapsed += Master::mpGameManager->GetTimeManager()->get_DeltaTime();
}

//*---------------------------------------------------------------------------------------
//*【?】描画処理
//*
//* [引数]なし
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeObject::Draw()
{
	VECTOR crntPos = mTransform.position;
	//DrawSphere3D(crntPos, m_Runtime.radius, 32, GetColor(255, 128, 128), GetColor(255, 255, 255), true);

	m_Visual.Draw(*this);
}


//*---------------------------------------------------------------------------------------
//*【?】シャドウ用描画処理
//*
//* [引数]なし
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeObject::DrawShadow()
{
	m_Visual.DrawShadow();
}



//*---------------------------------------------------------------------------------------
//*【?】見た目情報の更新
//*
//* [引数]なし
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeObject::RefreshVisual()
{
	m_Visual.RefreshMesh(*this);
}

//*---------------------------------------------------------------------------------------
//*【?】状態の初期化を行う（オブジェクトプール用）
//*
//* [引数]なし
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeObject::ResetForReuse()
{
	// トランスフォームリセット
	mTransform.Reset();	

	// 状態リセット
	m_Runtime.velocity = VEC_AllZero();
	m_Runtime.radius = 0.0f;
	m_Runtime.mergeCooldown = 0.0f;
	m_Runtime.isGrounded = false;
	m_Runtime.type = SLIME_TYPE::NORMAL;
	m_Runtime.splitPhase = SLIME_SPLIT_PHASE::PHASE_0;
	m_Runtime.isMoveDir = false;

	// 見た目リセット
	m_Visual.Reset();

	m_ReturnRequested = false;
}

//*---------------------------------------------------------------------------------------
//*【?】プールへの返却をリクエスト
//*
//* [引数]なし
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeObject::RequestReturn()
{
	m_ReturnRequested = true;
	SetActive(false);
}


//*---------------------------------------------------------------------------------------
//*【?】表情の設定
//*
//* [引数]
//* face : 表情番号
//* 
//* [返値]なし
//*---------------------------------------------------------------------------------------
void SlimeObject::SetFace(int face)
{
	m_Visual.SetFace(face);
}