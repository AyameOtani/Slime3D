#pragma once
#include <memory>
#include "Slime.h"
#include "ObjectPool.h"

// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:SlimeManager Class --- */
//
// 【?】スライムの管理を行う
//      生成や破棄は、ObjectManager側で行い、ここでその窓口になるようにする
//
// ***************************************************************************************
class SlimeManager
{
private:
    static constexpr int SLIME_POOL_CAPACITY = 1024;        // プールの最大容量
    static constexpr int SLIME_POOL_DEFAULT_CAPACITY = 256; // デフォルトで生成している数 

    // 固定データ定義
    // スライムの種類増加に対応できるよう配列で保持
    SlimeDef::SlimeDefinition 
        m_SlimeDefinition[UINT_CAST(SlimeDef::SLIME_TYPE::NUM)];

    // スライムプール
    std::unique_ptr<ObjectPool<Slime>> m_pSlimePool;

    // 生成したスライムたち
    std::vector<Slime*> m_SpawnedSlimes;        
private:
    // コピー禁止
    SlimeManager(const SlimeManager&) = delete;
    SlimeManager& operator=(const SlimeManager&) = delete;
    // ------------------------------------------------------


public:
    SlimeManager();
    ~SlimeManager();

    bool Setup();   // 初期化
    void Finalize();    // 終了
    void ProcessReturnRequests();   // プールへの返却状態をチェックする

    bool SpawnSlime(const SlimeDef::SlimeSpawnParams& spawnParams);      // スライムの生成
    void MergerSlime();     // スライムの合体
};

