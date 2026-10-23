#pragma once

#include "DxLib.h"
#include <array>

class Camera
{
public:
    /// <summary>
    /// カメラアングル用の設定データ
    /// </summary>
    struct CameraAngleSettingData
    {
        VECTOR positionOffset;  // 位置の補正値
        VECTOR targetOffset;    // 注視目標の補正値
    };

    /// <summary>
    /// カメラアングル  山本
    /// </summary>
    enum class CAMERA_ANGLE
    {
        ANGLE_A,    // 横斜め上から
        ANGLE_B,    // 後ろ斜め上から
        ANGLE_C,    // 上から

        NUM,
    };


    Camera();

 
    void Initialize( const VECTOR& playerPosition);   // 最初の準備    
    void Update( const VECTOR& playerPosition);// 毎フレーム更新
    void Apply() const; // カメラ設定をDxLibへ反映    
    void SetFollowRate( float rate );// 追従速度変更    
    VECTOR GetPosition() const;// カメラ位置取得   
    VECTOR GetTarget() const;// 注視点取得

    void SetCameraAngle(CAMERA_ANGLE angle) { m_CameraAngle = angle; }  // カメラアングルタイプの設定

private:
    // カメラ位置と注視点を作り直す
    void RebuildCamera();

    // カメラアングルデータのセットアップ
    void SetupCameraAngleSettingData();

private:
    float mCameraX;     // X方向の現在の追従位置    
    float mStageZ;      // 横スクロールなのでZ位置は固定する    
    float mFollowRate;  // 追従速度    
    VECTOR mPosition;   // 実際のカメラ位置   
    VECTOR mTarget;     // カメラが見る位置


    CAMERA_ANGLE m_CameraAngle;   // カメラアングルタイプ  山本
    std::array<                   // カメラアングルの設定データ配列
        CameraAngleSettingData,     
        static_cast<size_t>(CAMERA_ANGLE::NUM)> m_CameraAngleSettingDataList;

private:
    static constexpr float CAMERA_OFFSET_X = -0.80f;    // カメラをプレイヤーより少し左へ  
    static constexpr float CAMERA_Y = 10.0f;            // カメラの高さ
    static constexpr float CAMERA_DISTANCE_Z = 9.00f;   // プレイヤーからカメラまでの奥行き
    static constexpr float TARGET_OFFSET_X = 0.80f;     // プレイヤーより少し右側を見る
    static constexpr float TARGET_Y = 1.15f;            // 注視点の高さ
};
