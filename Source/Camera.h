#pragma once

#include "DxLib.h"
#include "Utility.h"
#include <array>

// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:Camera Class --- */
//
// 【?】カメラの処理を行う
//
// ***************************************************************************************
class Camera
{
public:
    // ================================
    // 列挙体
    // ================================
    /// @brief カメラアングル
    enum class CAMERA_ANGLE
    {
        ANGLE_A,    // 横斜め上から
        ANGLE_B,    // 後ろ斜め上から
        ANGLE_C,    // 上から

        NUM,
    };

    /// @brief カメラの操作タイプ
    enum class CONTROL_MODE
    {
        FIXED,  // 固定（従来のやつ）
        TPS,    // マウスで視点操作
    };

    // ================================
    // 構造体
    // ================================
    /// @brief カメラアングル用の設定データ
    struct CameraAngleSettingData
    {
        VECTOR positionOffset;  // 位置の補正値
        VECTOR targetOffset;    // 注視目標の補正値
    };

public:
    Camera();
 
    void Initialize( const VECTOR& playerPosition);     // 最初の準備    
    void Update( const VECTOR& playerPosition);         // 毎フレーム更新
    void Apply() const;                                 // カメラ設定をDxLibへ反映    
    void SetFollowRate( float rate );                   // 追従速度変更    
    VECTOR GetPosition() const;                         // カメラ位置取得   
    VECTOR GetTarget() const;                           // 注視点取得

    void SetCameraAngle(CAMERA_ANGLE angle) { m_CameraAngle = angle; }                          // カメラアングルタイプの設定
    void SetCameraFov(float fov_deg) { m_CameraFovRad = Helper::Math::DegToRad(fov_deg); }      // カメラの視野角の設定
    void SetCameraClip(float _near, float _far) { m_CameraNear = _near; m_CameraFar = _far; }   // カメラのクリッピング距離の設定

    void SetControlMode(CONTROL_MODE mode) { m_ControlMode = mode; }    // 操作タイプの設定
    CONTROL_MODE GetControlMode() const { return m_ControlMode; }       // 操作タイプの取得

    void SetTPSDistance(float dist) { m_TpsDistance = dist; };  // TPS操作時の対象からの距離
    float GetTPSDistance()const { return m_TpsDistance; }

    void SetMouseSensitivity(float sensitivity) { m_MouseSensitivity = sensitivity; };  // マウス感度の設定
    float GetMouseSensitivity()const { return m_MouseSensitivity; }
private:
    // カメラ位置と注視点を作り直す
    void RebuildCamera();
    // TPS操作用のカメラ情報を構築
    void RebuildTpsCamera(const VECTOR& playerPosition, float mouseDeltaX, float mouseDeltaY);

    // カメラアングルデータのセットアップ
    void SetupCameraAngleSettingData();

private:
    float mCameraX;     // X方向の現在の追従位置    
    float mStageZ;      // 横スクロールなのでZ位置は固定する    
    float mFollowRate;  // 追従速度    
    VECTOR mPosition;   // 実際のカメラ位置   
    VECTOR mTarget;     // カメラが見る位置

	float m_CameraFovRad;         // カメラの視野角 ラジアンで保持
	float m_CameraNear;           // カメラの近くの距離
	float m_CameraFar;            // カメラの遠くの距離

    float m_Yaw;                  // 水平方向の角度
    float m_Pitch;                // 上下方向の角度
    float m_TpsDistance;          // 注視点からの距離
    float m_MouseSensitivity;     // 1ピクセルあたりの回転角
    bool m_MouseLookReady;     
    CONTROL_MODE m_ControlMode;   // カメラの操作タイプ
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
