#pragma once
#include "DxLib.h"
#include "SlimeMesh.h"

// タイトル用スライムの設定値
// ジャンプやつぶれ具合、表示サイズなどを管理する
// 時間はフレーム数で扱う（60fps 想定）
namespace TitleSlimeConfig
{
    constexpr int kFramesPerSecond = 60;              // 1秒あたりのフレーム数
    constexpr int kJumpIntervalSeconds = 3;            // 自動ジャンプの間隔（秒）
    constexpr int kJumpIntervalFrames = kFramesPerSecond * kJumpIntervalSeconds; // 自動ジャンプの間隔（フレーム）

    constexpr float kJumpStartVelocity = 0.25f;       // ジャンプの初速
    constexpr float kGravity = 0.012f;                // 重力

    constexpr float kJumpStartSquash = 0.01f;         // ジャンプ開始時のつぶれ量
    constexpr float kLandingSquash = 0.01f;           // 着地時のつぶれ量

    constexpr float kSquashRest = 1.0f;               // 通常時の縦の倍率
    constexpr float kSquashSpring = 0.16f;            // 元の形に戻ろうとする強さ
    constexpr float kSquashDamping = 0.78f;           // 揺れの減衰率
    constexpr float kSquashMin = 0.62f;               // 縦の倍率の下限
    constexpr float kSquashMax = 1.45f;               // 縦の倍率の上限

    constexpr float kDefaultScale = 2.0f;             // 既定の表示サイズ

    constexpr float kFloorSearchDown = 1.0f;          // 床を探す範囲（下方向）
    constexpr float kFloorSearchUp = 0.5f;            // 床を探す範囲（上方向）

    constexpr int kBlendAlphaUnused = 0;              // 描画ブレンド（NOBLEND時は未使用）

    constexpr const char* kFaceImagePath = "Resource\\Slime\\face_a.png"; // 顔画像のパス
}

// タイトル用スライムのクラス
class TitleSlimeModel
{
public:
    TitleSlimeModel();                                // コンストラクタ
    ~TitleSlimeModel();                               // デストラクタ

    // SlimeMesh を値で持つため、コピーは禁止する
    TitleSlimeModel(const TitleSlimeModel&) = delete;
    TitleSlimeModel& operator=(const TitleSlimeModel&) = delete;

    void Initialize();                                // 初期化
    void Update();                                    // 自動ジャンプなどの更新
    void Draw();                                      // モデル描画

    void SetPosition(const VECTOR& pos);              // 表示位置の設定
    void SetScale(float scale);                       // 表示サイズの設定

private:
    // メンバ関数
    void UpdateJump();                                // ジャンプ・つぶれ具合の更新
    void UpdateGroundHeight();                        // 床の高さを取得
    void DrawFace(const VECTOR& drawPosition, float radius); // 顔の描画


    // メンバ類
    SlimeMesh mSlimeMesh;                             // 共通メッシュ

    VECTOR position;                                  // 表示の基準位置
    float groundY;                                    // 接地している床の高さ

    float jumpHeight;                                 // ジャンプの高さ
    float jumpVelocity;                               // ジャンプの上昇・下降速度
    int jumpTimer;                                    // 次のジャンプまでのフレーム数

    float modelScale;                                 // モデルの表示倍率

    float squashScale;                                // 体の縦方向の倍率
    float squashVelocity;                             // つぶれ・伸びの変化速度

    bool initialized;                                 // 初期化済みかどうか
    int mFaceHandle;                                  // 顔画像のハンドル
};