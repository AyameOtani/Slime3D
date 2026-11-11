#include "SlimeManager.h"

using namespace Game::Slime;

//*---------------------------------------------------------------------------------------
//*【?】コンストラクタ
//*----------------------------------------------------------------------------------------
SlimeManager::SlimeManager()
    : m_pSlimePool()
    , m_SlimeDefinition()
{

}

//*---------------------------------------------------------------------------------------
//*【?】デストラクタ
//*----------------------------------------------------------------------------------------
SlimeManager::~SlimeManager()
{

}

//*---------------------------------------------------------------------------------------
//*【?】初期化処理
//*
//* [引数] なし
//*
//* [返値]
//* true : 成功
//* false : 失敗
//*----------------------------------------------------------------------------------------
bool SlimeManager::Setup()
{
    // 二重にセットアップしない
    if (m_pSlimePool)
    {
        return true;
    }

    //
    // スライムの固定定義データ
    //
    m_SlimeDefinition[UINT_CAST(SLIME_TYPE::NORMAL)] =
    {
        {
            // 1.半径 2.移動速度 3.合体時間
             
            // 未分裂状態
            { 0.0f, 0.0f, 0.0f },
            
            // １段階目
            { 0.0f, 0.0f, 0.0f },
            
            // ２段階目
            { 0.0f, 0.0f, 0.0f },
            
            // ３段階目
            { 0.0f, 0.0f, 0.0f },
        },
    };

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    // スライムプールのセットアップ
    // 
    //////////////////////////////////////////////////////////////////////////////////////////
    m_pSlimePool = std::make_unique<ObjectPool<Slime>>(
        // 取得時に実行 ******************************************************************************************
        [&](Slime* obj) 
        {
            // 状態の初期化
            obj->ResetForReuse();       

            obj->SetDeleteFlag(false);
            obj->SetActive(true);
        },
        // 返却時に実行 ******************************************************************************************
        [](Slime* obj)
        {
            obj->SetActive(false);
        },
        // 生成時に実行 ******************************************************************************************
        [&]()->Slime*
        {
            Slime* obj = new Slime();
            obj->SetActive(false);
            return obj;
        },
        SLIME_POOL_DEFAULT_CAPACITY,    // 初期生成数
        SLIME_POOL_CAPACITY             // 最大生成数
    );

    m_SpawnedSlimes.clear();

	return true;
}

//*---------------------------------------------------------------------------------------
//*【?】終了処理
//*
//* [引数] なし
//* [返値] なし
//*----------------------------------------------------------------------------------------
void SlimeManager::Finalize()
{
    // プールの開放
    m_pSlimePool->clear();

    m_SpawnedSlimes.clear();
}

//*---------------------------------------------------------------------------------------
//*【?】プールへの返却状態をチェックする
//*
//* [引数] なし
//* [返値] なし
//*----------------------------------------------------------------------------------------
void SlimeManager::ProcessReturnRequests()
{
    // プールが生成されているか
    if (!m_pSlimePool)
    {
        return;
    }

    //
    // プールへの返却フラグの確認
    //
    for (auto it = m_SpawnedSlimes.begin();
        it != m_SpawnedSlimes.end();)
    {
        Slime* obj = *it;

        // フラグが立ってないなら、次へ
        if (!obj->IsReturnRequested())
        {
            ++it;
            continue;
        }

        // プールへ返却する
        m_pSlimePool->release(obj);

        // 配列からも削除
        it = m_SpawnedSlimes.erase(it);
    }
}

//*---------------------------------------------------------------------------------------
//*【?】スライムを出現させる
//*
//* [引数] 
//* spawnParams : 出現させるためのパラメータ
//* 
//* [返値] 
//* 出現成功したか 
//*----------------------------------------------------------------------------------------
bool SlimeManager::SpawnSlime(const SlimeDef::SlimeSpawnParams& spawnParams)
{
    // プールが生成されているか
    if (!m_pSlimePool)
    {
        Helper::Debug::ErrorMessage(L"スライムプールが生成されていませんです", L"SlimeManager::SpawnSlime");
        return false;
    }

    const auto typeIndex = UINT_CAST(spawnParams.type);
    const auto phaseIndex = UINT_CAST(spawnParams.splitPhase);
    
    // 存在するスライムか
    if (typeIndex >= UINT_CAST(SLIME_TYPE::NUM) ||
        phaseIndex >= UINT_CAST(SLIME_SPLIT_PHASE::NUM))
    {
        Helper::Debug::ErrorMessage(L"存在しないスライムです", L"SlimeManager::SpawnSlime");
        return false;
    }

    // プールからスライムオブジェクトを取得する
    Slime* obj = m_pSlimePool->get();

    // 取得できなかった
    if (!obj)
    {
        Helper::Debug::ErrorMessage(L"スライムのインスタンスが取得できませんでした",L"SlimeManager::SpawnSlime");
        return false;
    }

    // スライムのサイズごとの固定データを取り出す
    const auto& definition =
        m_SlimeDefinition[typeIndex].sizes[phaseIndex];

    
    // 初期化など
    obj->Init(&definition, spawnParams.splitPhase);
    obj->SetPosition(spawnParams.position);
    obj->SetDirection(spawnParams.direction);
    obj->SetActive(true);

    // 生成したスライムを保持する
    m_SpawnedSlimes.push_back(obj);

    return true;
}
