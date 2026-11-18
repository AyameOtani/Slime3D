#pragma once
#include "DxLib.h"
#include "SlimeModel.h"
#include "GameDefinitions.h"
#include <memory>
#include <string>
#include <vector>

// MQOの三角形をそのまま使う静的ステージ。斜面にも対応。
class Stage
{
public:
    Stage();
    ~Stage();
    Stage(const Stage&) = delete;
    Stage& operator=(const Stage&) = delete;
    bool Initialize(const char* path = "Resource\\Stage\\stage01.mqo", float scale = 1.0f, VECTOR position = VGet(0, 0, 0));
    void Finalize();
    void Update(const SlimeModel& slime);
    void Draw();
    void DrawPaint();   // ペイントの描画 追加 山本
    const std::string& GetError() const;
    const std::vector<Game::BoxCollider>& GetBoxes() const;
    static Stage* Active();
    bool FindFloor(float x, float z, float minY, float maxY, float& y, VECTOR* normal = nullptr) const;
    bool BabyCanStand(float x, float z, float currentY, float radius, float& y) const;
    void ResolveSphere(VECTOR& position, VECTOR& velocity, float radius, bool& grounded, float& impact, VECTOR& wall, float& wallDepth, Game::SurfaceHit* groundHit = nullptr) const;
    // color: 0=虹色、1=赤、2=青、3=緑、4=元の色の復元
    void Paint(VECTOR position, float contactRadius, float brushRadius, int color);


    // 塗る
    void ApplyPaint(const Game::SurfaceHit& hit, float radius, Game::PAINT_COLOR color);

    // 掃除
    double CleanPaint(const Game::SurfaceHit& hit, float radius);

    // 塗った範囲情報の取得
    Game::PaintCoverage GetPaintCoverage() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
