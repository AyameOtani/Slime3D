#include "Slime.h"
#include "Master.h"

using namespace Game::Slime;
using namespace Helper::DxLib_Math;

//*---------------------------------------------------------------------------------------
//*【?】コンストラクタ
//*----------------------------------------------------------------------------------------
Slime::Slime()
	: Object3D()
	, m_Runtime()
	, m_pSizeDefinition(nullptr)
	, m_ReturnRequested(false)
{

}

//*---------------------------------------------------------------------------------------
//*【?】デストラクタ
//*----------------------------------------------------------------------------------------
Slime::~Slime()
{

}

//*---------------------------------------------------------------------------------------
//*【?】スライムの初期化
//*
//* [引数]
//* definition : スライムの固定データの参照
//* splitPhase : 分裂の状態 
//* 
//* [返値]なし
//*----------------------------------------------------------------------------------------
void Slime::Init(const SlimeSizeDefinition* definition, const SLIME_SPLIT_PHASE splitPhase)
{
	m_pSizeDefinition = definition;
	m_Runtime.splitPhase = splitPhase;
	m_Runtime.radius = m_pSizeDefinition->radius;
}

//*---------------------------------------------------------------------------------------
//*【?】更新処理
//*
//* [引数]なし
//* [返値]なし
//*----------------------------------------------------------------------------------------
void Slime::Update()
{
	VECTOR moveDir = VGet(0.0f, 0.3f, 1.0f);
	m_Runtime.velocity = VScale(moveDir, m_pSizeDefinition->moveSpeed);
	VECTOR crntPos = mTransform.position;
	VECTOR newPos = VAdd(crntPos, m_Runtime.velocity);
	mTransform.position = newPos;

	if (m_Elapsed >= 1.0f)
	{
		RequestReturn();

		int handle = Master::mpEffectManager->PlayEffect3D("SlimeExplosion");
		Master::mpEffectManager->SetPosEffect3D(handle,crntPos);
		Master::mpEffectManager->SetScaleEffect3D(handle, VEC_GetScalar(1.0f));

		SlimeDef::SlimeSpawnParams spawn;
		spawn.position = VGet(0.0f, 0.0f, 0.0f);
		spawn.type = SLIME_TYPE::NORMAL;
		spawn.splitPhase = SLIME_SPLIT_PHASE::PHASE_0;
		Master::mpGameManager->GetSlimeManager()->SpawnSlime(spawn);
	}

	m_Elapsed += 0.01f;
}

//*---------------------------------------------------------------------------------------
//*【?】描画処理
//*
//* [引数]なし
//* [返値]なし
//*----------------------------------------------------------------------------------------
void Slime::Draw()
{
	VECTOR crntPos = mTransform.position;
	DrawSphere3D(crntPos, m_Runtime.radius, 32, GetColor(255, 128, 128), GetColor(255, 255, 255), true);
}


//*---------------------------------------------------------------------------------------
//*【?】状態の初期化を行う（オブジェクトプール用）
//*
//* [引数]なし
//* [返値]なし
//*----------------------------------------------------------------------------------------
void Slime::ResetForReuse()
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

	m_ReturnRequested = false;

	m_Elapsed = 0.0f;
}

//*---------------------------------------------------------------------------------------
//*【?】プールへの返却をリクエスト
//*
//* [引数]なし
//* [返値]なし
//*----------------------------------------------------------------------------------------
void Slime::RequestReturn()
{
	m_ReturnRequested = true;
	SetActive(false);
}
