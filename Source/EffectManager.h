#pragma once
#include "DxLib.h"
#include <unordered_map>
#include <string>

// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:EffectManager Class --- */
//
//  ★★★シングルトン★★★
//
// 【?】エフェクトの管理
//      Effekseerのエフェクトの読み込み、再生等を行う
// 
// ***************************************************************************************
class EffectManager
{
private:
    std::unordered_map<std::string, int> m_EffectHandleMap; // エフェクトのハンドルマップ（文字列をキーにする）


public:
    EffectManager();
    ~EffectManager();

    bool Setup();
    bool Release();
    void EffectUpdate3D();
    void EffectDraw3D();   
    void EffectUpdate2D();
    void EffectDraw2D();


    //*****************************************************************************************
    //						外部からエフェクトにアクセスする用の関数群
    //          エフェクトのロードや、位置の変更などはここの関数を使用してください。
    //*****************************************************************************************
    int LoadEffect(const std::string& filePath, float magnification = 1.0f);                // エフェクトのロード
    int PlayEffect3D(int effectHandle);                                                     // エフェクトの再生
    bool SetPosEffect3D(int playingEffectHandle, float x, float y, float z);                // 再生中の3D表示のエフェクトの位置を設定
    bool SetPosEffect3D(int playingEffectHandle, const VECTOR& pos);                        // 再生中の3D表示のエフェクトの位置を設定
    bool SetRotationEffect3D(int playingEffectHandle, float x, float y, float z);           // 再生中の3D表示のエフェクトの角度を設定
    bool SetRotationEffect3D(int playingEffectHandle, const VECTOR& rot);                   // 再生中の3D表示のエフェクトの角度を設定   
    bool SetScaleEffect3D(int playingEffectHandle, float x, float y, float z);              // 再生中の3D表示のエフェクトの拡大率を設定
    bool SetScaleEffect3D(int playingEffectHandle, const VECTOR& rot);                      // 再生中の3D表示のエフェクトの拡大率を設定
    bool SetSpeedEffect3D(int playingEffectHandle, float speed);                            // 再生中の3D表示のエフェクトの再生速度を設定
    void SetDynamicParameterEffect3D(int playingEffectHandle, int32_t index, float value);  // エフェクトの動的パラメータを設定する
    bool IsPlayingEffect3D(int playingEffectHandle);


private:
    // コピー禁止
    EffectManager(const EffectManager&) = delete;
    EffectManager& operator=(const EffectManager&) = delete;
};

