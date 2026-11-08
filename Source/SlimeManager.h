#pragma once


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
    // コピー禁止
    SlimeManager(const SlimeManager&) = delete;
    SlimeManager& operator=(const SlimeManager&) = delete;
    // ------------------------------------------------------


public:
    SlimeManager();
    ~SlimeManager();

    bool Setup();   // 初期化
    void Term();    // 終了
};

