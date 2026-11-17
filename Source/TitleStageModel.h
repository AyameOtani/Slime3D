#pragma once
#include "DxLib.h"
#include "Stage.h"

// タイトル用ステージの設定値
// 読み込むモデルのパスや表示位置、倍率を管理する
namespace TitleStageConfig
{
    constexpr const char* kStageModelPath = "Resource\\Stage\\stage01.mqo"; // ステージモデルのパス
    constexpr float kStageScale = 1.0f;               // ステージの表示倍率

    // 表示位置（スライムと同じ奥行きに合わせる）
    constexpr float kStagePosX = 0.0f;                // ステージの表示位置X
    constexpr float kStagePosY = 0.0f;                // ステージの表示位置Y
    constexpr float kStagePosZ = 300.0f;              // ステージの表示位置Z（スライムの位置と同じ）
}

// タイトル用ステージのクラス
class TitleStageModel
{
public:
    TitleStageModel();                                // コンストラクタ
    ~TitleStageModel();                               // デストラクタ

    // Stage を値で持つため、コピーは禁止する
    TitleStageModel(const TitleStageModel&) = delete;
    TitleStageModel& operator=(const TitleStageModel&) = delete;

    void Initialize();                                // 初期化（ステージの読み込み）
    void Draw();                                      // ステージ描画
    void Finalize();                                  // 終了処理（ステージの解放）

    bool IsLoaded() const;                            // ステージが読み込めているかどうか

private:
    Stage mStage;                                     // ステージ本体

    bool loaded;                                      // 読み込み済みかどうか
};