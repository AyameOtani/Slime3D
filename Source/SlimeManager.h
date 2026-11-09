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
    Slime::SlimeDefinition 
        m_SlimeDefinition[UINT_CAST(Slime::SLIME_TYPE::NUM)];

    // スライムプール
    std::unique_ptr<ObjectPool<Slime>> m_pSlimePool;

private:
    // コピー禁止
    SlimeManager(const SlimeManager&) = delete;
    SlimeManager& operator=(const SlimeManager&) = delete;
    // ------------------------------------------------------


public:
    SlimeManager();
    ~SlimeManager();

    bool Setup();   // 初期化
    void Term();    // 終了

    void SpawnSlime();      // スライムの生成
    void ReleaseSlime();    // スライムの返却
    void MergerSlime();     // スライムの合体
};

