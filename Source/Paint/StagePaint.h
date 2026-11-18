#pragma once
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "../GameDefinitions.h"

// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:StagePaint Class --- */
//
// 【?】
//
// ***************************************************************************************
class StagePaint
{
private:
    struct Face
    {
        VECTOR a, b, c;
        VECTOR normal{};                    // 描画時に床の上へ少し浮かせる方向
        int divisions = 1;
        double cellArea = 0.0;
        float longestEdge = 0.0f;           // ブラシから遠い領域を除外するために使う


        std::vector<std::uint8_t> cells;  // セル番号 → 色。0 は未塗装
        std::vector<int> slotForCell;     // セル番号 → 描画位置。-1 は未塗装
        std::vector<int> cellForSlot;     // 描画位置 → セル番号
        std::vector<VERTEX3D> vertices;   // 着色セル1個につき連続する3頂点
    };

    std::unordered_map<int, Face> m_Faces;
    Game::PaintCoverage m_Coverage{};
    std::int64_t m_PaintedCellCount = 0;  // ステージ全体の着色セル数
public:
    void RegisterTriangle(int triangleId, const VECTOR& a, const VECTOR& b, const VECTOR& c);
    void Clear();
    void ApplyToTriangle(int triangleId, const VECTOR& center, float radius, Game::PAINT_COLOR color);
    double CleanTriangle(int triangleId, const VECTOR& center, float radius);
    Game::PaintCoverage GetPaintCoverage() const;
    void Draw();

private:
    double EditTriangle(int triangleId, const VECTOR& center, float radius, std::uint8_t ink);
};

