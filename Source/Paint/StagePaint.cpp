#include "StagePaint.h"
#include <algorithm>
#include <cmath>
#include <utility>

using namespace Game;

namespace
{
    struct CellTriangle
    {
        VECTOR a, b, c;
    };

    struct Brush
    {
        VECTOR center;
        float radius;
    };

    VECTOR Midpoint(VECTOR a, VECTOR b)
    {
        return VScale(VAdd(a, b), 0.5f);
    }

    template<class Visitor>
    void VisitCells(const CellTriangle& t, int divisions, int firstCell,
        float longestEdge, const Brush* brush, const Visitor& visit)
    {
        if (brush)
        {
            // この三角形全体は、a を中心、最長辺を半径とする球の中にある
            const float reach = brush->radius + longestEdge;
            if (VSquareSize(VSub(brush->center, t.a)) > reach * reach)
                return;
        }

        if (divisions == 1)
        {
            if (brush)
            {
                const VECTOR center = VScale(VAdd(VAdd(t.a, t.b), t.c), 1.0f / 3.0f);
                if (VSquareSize(VSub(center, brush->center)) >
                    brush->radius * brush->radius)
                    return;
            }

            visit(firstCell, t);
            return;
        }

        const VECTOR ab = Midpoint(t.a, t.b);
        const VECTOR bc = Midpoint(t.b, t.c);
        const VECTOR ca = Midpoint(t.c, t.a);

        const int half = divisions / 2;
        const int childCells = half * half;
        const float childEdge = longestEdge * 0.5f;

        VisitCells({ t.a, ab, ca }, half, firstCell, childEdge, brush, visit);
        VisitCells({ ab, t.b, bc }, half, firstCell + childCells, childEdge, brush, visit);
        VisitCells({ ca, bc, t.c }, half, firstCell + 2 * childCells, childEdge, brush, visit);
        VisitCells({ ab, bc, ca }, half, firstCell + 3 * childCells, childEdge, brush, visit);
    }


    COLOR_U8 CellColor(VECTOR p, std::uint8_t bits)
    {
        if ((bits & 8) == 0)
        {
            // ワールド座標から、なだらかな明暗のムラを作ri、質感を表現
            const float variation =
                std::sin(p.x * 4.0f + p.z * 2.0f) *
                std::sin(p.z * 5.0f - p.x * 2.0f);
            const float brightness = 1.0f + 0.1f * variation;

            const auto shade = [brightness](int channel) {
                const float value = channel * brightness;
                return static_cast<int>((std::min)(255.0f, value) + 0.5f);
                };

            return GetColorU8(
                shade((bits & 1) ? 245 : 25),
                shade((bits & 4) ? 225 : 25),
                shade((bits & 2) ? 245 : 25),
                255);
        }

        static const int colors[7][3] = {
            {245,35,45}, {255,125,15}, {250,230,20}, {30,205,65},
            {10,200,235}, {35,75,240}, {155,35,225}
        };
        const float phase = (p.x + 0.45f * p.z + 0.3f * p.y) * (2.0f * DX_PI_F / 2.4f);
        const float t = (0.5f - 0.5f * std::cos(phase)) * 6.0f;
        const int i = (std::min)(5, static_cast<int>(t));
        const float f = t - i;

        auto channel = [&](int k) {
            return static_cast<int>(colors[i][k] + (colors[i + 1][k] - colors[i][k]) * f);
            };
        return GetColorU8(channel(0), channel(1), channel(2), 255);
    }
}

//*---------------------------------------------------------------------------------------
//*【?】ペイントの適用
//*
//* [引数]
//* hit    : 衝突情報
//* radius : 範囲
//* color  : 何色で塗るか
//*
//* [返値] なし
//*----------------------------------------------------------------------------------------
void StagePaint::
RegisterTriangle(int triangleId, const VECTOR& a, const VECTOR& b, const VECTOR& c)
{
    if (triangleId < 0 || m_Faces.count(triangleId) != 0)
        return;

    const VECTOR ab = VSub(b, a);
    const VECTOR ac = VSub(c, a);
    const VECTOR cross = VCross(ab, ac);
    const float twiceArea = VSize(cross);

    if (!(twiceArea > 0.0f) || !std::isfinite(twiceArea))
        return;

    Face face{};
    face.a = a;
    face.b = b;
    face.c = c;
    face.normal = VScale(cross, 1.0f / twiceArea);

    // 現在の対象は床と斜面。モデルの頂点順にかかわらず上側へ描画する。
    if (face.normal.y < 0.0f)
        face.normal = VScale(face.normal, -1.0f);

    face.longestEdge = (std::max)({ VSize(ab), VSize(ac), VSize(VSub(c, b)) });

    constexpr float targetCellEdge = 0.1f;  // セルの一辺の長さ（大きいほど粗くなる）
    constexpr int maxDivisions = 32;        // 元の三角形の辺を何分割するか
    while (face.divisions < maxDivisions &&
        face.longestEdge > targetCellEdge * face.divisions)
    {
        face.divisions *= 2;
    }

    const double area = static_cast<double>(twiceArea) * 0.5;
    face.cellArea = area / (face.divisions * face.divisions);

    m_Faces.emplace(triangleId, std::move(face));
    m_Coverage.paintableArea += area;
}


void StagePaint::
ApplyToTriangle(int triangleId, const VECTOR& center, float radius, Game::PAINT_COLOR color)
{
    std::uint8_t ink;
    switch (color)
    {
    case Game::PAINT_COLOR::RED:     ink = 1; break;
    case Game::PAINT_COLOR::BLUE:    ink = 2; break;
    case Game::PAINT_COLOR::GREEN:   ink = 4; break;
    case Game::PAINT_COLOR::RAINBOW: ink = 8; break;
    default: return;
    }

    EditTriangle(triangleId, center, radius, ink);
}

double StagePaint::
CleanTriangle(int triangleId, const VECTOR& center, float radius)
{
    return EditTriangle(triangleId, center, radius, 0);
}

void StagePaint::Draw()
{
    const int previousCulling = GetUseBackCulling();
    SetUseBackCulling(DX_CULLING_NONE);

    for (const auto& entry : m_Faces)
    {
        const Face& face = entry.second;
        if (face.vertices.empty())
            continue;

        DrawPolygon3D(face.vertices.data(),
            static_cast<int>(face.cellForSlot.size()),
            DX_NONE_GRAPH, TRUE);
    }

    SetUseBackCulling(previousCulling);
}

void StagePaint::Clear()
{
    m_Faces.clear();
    m_Coverage = {};
    m_PaintedCellCount = 0;
}

PaintCoverage StagePaint::
GetPaintCoverage() const
{
    return m_Coverage;
}

double StagePaint::
EditTriangle(int triangleId, const VECTOR& center, float radius, std::uint8_t ink)
{
    auto it = m_Faces.find(triangleId);
    if (it == m_Faces.end() || !std::isfinite(radius) || radius <= 0.0f)
        return 0.0;

    Face& face = it->second;
    if (ink == 0 && face.cellForSlot.empty())
        return 0.0;

    const int countBefore = static_cast<int>(face.cellForSlot.size());
    const VECTOR offset = VScale(face.normal, 0.016f);
    const Brush brush{ center, radius };

    VisitCells({ face.a, face.b, face.c }, face.divisions, 0,
        face.longestEdge, &brush, [&](int index, const CellTriangle& triangle)
        {
            if (face.cells.empty())
            {
                const int cellCount = face.divisions * face.divisions;
                face.cells.assign(cellCount, 0);
                face.slotForCell.assign(cellCount, -1);
            }

            // 既存の混色規則。0 は掃除、8 は虹、1・2・4 は赤・青・緑。
            const std::uint8_t before = face.cells[index];
            std::uint8_t after = 0;
            if (ink == 8)
                after = static_cast<std::uint8_t>(before | 8);
            else if (ink != 0)
                after = static_cast<std::uint8_t>((before & 7) | ink);

            if (before == after)
                return;

            int slot = face.slotForCell[index];

            // 掃除。末尾のセルを空いた位置へ移す。
            if (after == 0)
            {
                const int lastSlot = static_cast<int>(face.cellForSlot.size()) - 1;

                if (slot != lastSlot)
                {
                    std::copy_n(face.vertices.data() + lastSlot * 3, 3,
                        face.vertices.data() + slot * 3);

                    const int movedCell = face.cellForSlot[lastSlot];
                    face.cellForSlot[slot] = movedCell;
                    face.slotForCell[movedCell] = slot;
                }

                face.vertices.resize(lastSlot * 3);
                face.cellForSlot.pop_back();
                face.slotForCell[index] = -1;
                face.cells[index] = 0;
                return;
            }

            // 新規塗装。新しいセルだけ、頂点3個分の場所を取る
            if (before == 0)
            {
                slot = static_cast<int>(face.cellForSlot.size());
                face.cellForSlot.push_back(index);
                face.slotForCell[index] = slot;
                face.vertices.resize(face.vertices.size() + 3);
            }

            const VECTOR points[3] = { triangle.a, triangle.b, triangle.c };
            for (int k = 0; k < 3; ++k)
            {
                VERTEX3D& vertex = face.vertices[slot * 3 + k];

                if (before == 0)
                {
                    vertex.pos = VAdd(points[k], offset);
                    vertex.norm = face.normal;
                }

                // 元の床座標で色を求めるので、虹の模様も従来と同じ
                vertex.dif = CellColor(points[k], after); 
                vertex.spc = GetColorU8(140, 140, 140, 255);
            }

            face.cells[index] = after;
        });

    const int countAfter = static_cast<int>(face.cellForSlot.size());
    const int cellDelta = countAfter - countBefore;
    const double areaDelta = cellDelta * face.cellArea;

    m_PaintedCellCount += cellDelta;
    m_Coverage.dirtyArea += areaDelta;
    if (m_PaintedCellCount == 0)
        m_Coverage.dirtyArea = 0.0;

    return std::abs(areaDelta);
}
