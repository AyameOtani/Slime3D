#pragma once
#include "DxLib.h"

// タイトル用カメラの設定値
// 注視点を固定し、その周りを水平方向に回転させる
namespace TitleCameraConfig
{
    constexpr float kFovDeg = 50.0f;          // 視野角（度）
    constexpr float kNear = 0.1f;             // 描画の手前の距離
    constexpr float kFar = 1500.0f;           // 描画の奥の距離

    constexpr float kRadius = 20.0f;          // 注視点からカメラまでの水平距離
    constexpr float kHeight = 9.0f;           // カメラの高さ
    constexpr float kTargetY = 1.35f;         // 注視点の高さ

    constexpr float kRotateSpeed = 0.004f;     // 1フレームあたりの回転量（ラジアン）
}

// タイトル用カメラのクラス
// 注視点を固定して、水平方向に回転し続ける
class TitleCamera
{
public:
    TitleCamera();                                    // コンストラクタ
    ~TitleCamera();                                   // デストラクタ

    void Initialize(const VECTOR& targetPosition);    // 初期化（注視点の設定）
    void Update();                                    // 回転の更新とカメラの反映
    void Apply() const;                               // カメラ設定をDxLibへ反映

    VECTOR GetPosition() const;                       // カメラ位置の取得
    VECTOR GetTarget() const;                         // 注視点の取得

private:
    void RebuildPosition();                           // 現在の角度からカメラ位置を作り直す

    VECTOR mTarget;                                   // 注視点（固定）
    VECTOR mPosition;                                 // カメラ位置
    float mYaw;                                       // 水平方向の角度（ラジアン）
};