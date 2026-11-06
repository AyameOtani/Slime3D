#include "SlimeMesh.h"
#include <cmath>

// 元の SlimeModel.cpp の CreateMesh / UpdateMesh / RainbowColor から移した処理。
// 数値・処理順は元のまま。本編固有の変形は SlimeModel 側に残す。

const float SlimeMesh::BODY_WIDTH = 1.20f;
const float SlimeMesh::BODY_HEIGHT = 1.75f;
const float SlimeMesh::DEFAULT_RADIUS = 0.95f;

namespace
{
    // 段の形状の区切り位置（元の UpdateMesh と同じ値）
    const float BELLY_START = 0.55f;   // 最も横に広がる位置
    const float BASE_START = 0.90f;    // 丸い下半身から平らな底面へ切り替わる位置
    const float BELLY_HEIGHT = 0.32f;  // お腹の高さ（全高に対する割合）
    const float BASE_RADIUS = 0.42f;   // 底面の半径（最大幅に対する割合）

    float ClampValue(float value, float minValue, float maxValue)
    {
        if (value < minValue) return minValue;
        if (value > maxValue) return maxValue;
        return value;
    }

    // 上から赤・橙・黄・緑・水色・青・紫をすべて通る。
    // 高さを主成分にして、頭と底でも虹の端の色が欠けないようにする。
    COLOR_U8 RainbowColor(float horizontal, float heightRate)
    {
        const int colors[7][3] = {
            {245, 35, 45}, {255, 125, 15}, {250, 230, 20},
            {30, 205, 65}, {10, 200, 235}, {35, 75, 240}, {155, 35, 225}
        };

        float h = heightRate < 0.0f ? 0.0f : (heightRate > 1.0f ? 1.0f : heightRate);
        float t = 1.0f - h + (horizontal - 0.5f) * 0.32f * sinf(h * DX_PI_F);
        t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
        float segment = t * 6.0f;
        int i = static_cast<int>(segment);

        if (i > 5) i = 5;

        float f = segment - i;

        return GetColorU8(static_cast<int>(colors[i][0] + (colors[i + 1][0] - colors[i][0]) * f), static_cast<int>(colors[i][1] + (colors[i + 1][1] - colors[i][1]) * f), static_cast<int>(colors[i][2] + (colors[i + 1][2] - colors[i][2]) * f), 255);
    }

    // ベクトルの外積
    VECTOR CrossVector(const VECTOR& a, const VECTOR& b)
    {
        return VGet(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }
}

SlimeMesh::SlimeMesh()
{
}

void SlimeMesh::Initialize()
{
    vertices.clear();
    indices.clear();

    vertices.resize((LAT + 1) * (LON + 1));

    for (int y = 0; y < LAT; ++y)
    {
        for (int x = 0; x < LON; ++x)
        {
            // 行番号×1行の点数＋列番号で、2次元の位置を1つの番号に変換する。
            // i0・i1が今の段の隣り合う点、i2・i3が次の段の点。
            int i0 = y * (LON + 1) + x;
            int i1 = i0 + 1;
            int i2 = i0 + (LON + 1);
            int i3 = i2 + 1;

            // 4つの点を2枚の三角形に分ける。indicesには座標ではなく頂点番号を保存する。
            // 最初は(i0,i2,i1)、次は(i1,i2,i3)。並べる順番が面の向きに関わる。
            indices.push_back(static_cast<unsigned short>(i0));
            indices.push_back(static_cast<unsigned short>(i2));
            indices.push_back(static_cast<unsigned short>(i1));

            indices.push_back(static_cast<unsigned short>(i1));
            indices.push_back(static_cast<unsigned short>(i2));
            indices.push_back(static_cast<unsigned short>(i3));
        }
    }
}

std::vector<VERTEX3D>& SlimeMesh::GetVertices()
{
    return vertices;
}

const std::vector<VERTEX3D>& SlimeMesh::GetVertices() const
{
    return vertices;
}

const std::vector<unsigned short>& SlimeMesh::GetIndices() const
{
    return indices;
}

void SlimeMesh::ComputeRing(int row, RingInfo& ring)
{
    const float v = static_cast<float>(row) / LAT;

    // 各高さに輪を作り、積み重ねて体にする。ringRadiusは輪の半径、heightFromBottomは底からの高さ。
    float ringRadius = 0.0f;
    float heightFromBottom = 0.0f;

    if (v <= BELLY_START)
    {
        // ------------------------------------------------
        // 頭の先端 → お腹
        // 上は尖り、お腹に向かって滑らかに広がる
        // ------------------------------------------------
        float t = v / BELLY_START;

        ringRadius = BODY_WIDTH * sinf(t * DX_PI_F * 0.5f);

        heightFromBottom = BODY_HEIGHT * (BELLY_HEIGHT + (1.0f - BELLY_HEIGHT) * (1.0f - t));
    }
    else if (v <= BASE_START)
    {
        // ------------------------------------------------
        // お腹 → 足元
        // 楕円のカーブで下側を丸く絞る
        // ------------------------------------------------
        float t = (v - BELLY_START) / (BASE_START - BELLY_START);
        float angle = t * DX_PI_F * 0.5f;

        ringRadius = BODY_WIDTH * (BASE_RADIUS + (1.0f - BASE_RADIUS) * cosf(angle));

        heightFromBottom = BODY_HEIGHT * BELLY_HEIGHT * (1.0f - sinf(angle));
    }
    else
    {
        // ------------------------------------------------
        // 小さな平らな底面
        // 接地部分だけ平らにして安定感を出す
        // ------------------------------------------------
        float t = (v - BASE_START) / (1.0f - BASE_START);

        ringRadius = BODY_WIDTH * BASE_RADIUS * (1.0f - t);
        heightFromBottom = 0.0f;
    }

    // 底なら0、頭なら1になる高さの割合。揺らす場所・垂らす場所の強さを分ける。
    float heightRate = heightFromBottom / BODY_HEIGHT;

    // 底面は揺らさず、上側ほど揺らす
    float upperWeight = heightRate * heightRate;

    // 下側ほど垂れやすくする
    float lowerWeight = ClampValue(1.0f - heightRate / 0.45f, 0.0f, 1.0f);
    lowerWeight *= lowerWeight;

    ring.ringRadius = ringRadius;
    ring.heightFromBottom = heightFromBottom;
    ring.heightRate = heightRate;
    ring.upperWeight = upperWeight;
    ring.lowerWeight = lowerWeight;
}

void SlimeMesh::ComputeDirection(int col, float& dirX, float& dirZ)
{
    float u = static_cast<float>(col) / LON;

    // 両端の座標を完全に一致させる
    float phi = (col == LON) ? 0.0f : u * DX_PI_F * 2.0f;

    // cosとsinで輪の周りの向きを作る。その向きに輪の半径を掛けると円周上の点になる。
    dirX = cosf(phi);
    dirZ = sinf(phi);
}

float SlimeMesh::ComputeColorHorizontal(float dirX, float ringRadius)
{
    return 0.5f + dirX * ringRadius / (BODY_WIDTH * 2.0f);
}

void SlimeMesh::SetVertex(int row, int col, const VECTOR& pos, float heightRate, float horizontal)
{
    int index = row * (LON + 1) + col;
    if (index < 0 || index >= static_cast<int>(vertices.size())) return;

    VERTEX3D& vertex = vertices[index];

    vertex.pos = pos;

    // 法線はこの後、実際の形から再計算する
    vertex.norm = VGet(0.0f, 0.0f, 0.0f);

    // 本体は常に虹色。数字キーの選択色は赤ちゃんにだけ使う。
    // difは頂点の色(R,G,B,A)。Aは不透明度で、255が不透明。spcは光沢の色。
    vertex.dif = RainbowColor(horizontal, heightRate);

    vertex.spc = GetColorU8(18, 18, 18, 255);

    // u・vは画像を貼るための座標。今回の描画では画像を指定していない。
    vertex.u = static_cast<float>(col) / LON;
    vertex.v = static_cast<float>(row) / LAT;

    vertex.su = 0.0f;
    vertex.sv = 0.0f;
}

void SlimeMesh::BuildBaseShape(const VECTOR& position, float radius, float scaleY)
{
    if (vertices.empty())
    {
        return;
    }

    float scaleXZ = 1.0f / sqrtf(scaleY);

    for (int row = 0; row <= LAT; ++row)
    {
        RingInfo ring;
        ComputeRing(row, ring);

        for (int col = 0; col <= LON; ++col)
        {
            float directionX = 0.0f;
            float directionZ = 0.0f;
            ComputeDirection(col, directionX, directionZ);

            // 底面は当たり判定の下端に合わせる。
            // 縦に伸縮しても接地位置を変えない。
            // Yを-radiusから作るので、底面を基準に上方向へ伸縮できる。
            VECTOR local = VGet(
                directionX * ring.ringRadius * radius * scaleXZ,
                -radius + ring.heightFromBottom * radius * scaleY,
                directionZ * ring.ringRadius * radius * scaleXZ);

            SetVertex(
                row,
                col,
                VAdd(position, local),
                ring.heightRate,
                ComputeColorHorizontal(directionX, ring.ringRadius));
        }
    }

    RecalculateNormals(radius);
}

void SlimeMesh::RecalculateNormals(float radius)
{
    if (vertices.empty())
    {
        return;
    }

    // 面の法線を集める
    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        int i0 = indices[i];
        int i1 = indices[i + 1];
        int i2 = indices[i + 2];

        VECTOR edge1 = VSub(vertices[i1].pos, vertices[i0].pos);
        VECTOR edge2 = VSub(vertices[i2].pos, vertices[i0].pos);

        // 現在のインデックス順に対して外向きになる順序
        VECTOR faceNormal = CrossVector(edge2, edge1);

        // 頭頂部や底面中央の面積ゼロの三角形は無視
        if (VSquareSize(faceNormal) < 0.000000000001f)
        {
            continue;
        }

        // この面の法線を3頂点に足す。隣の面の向きも集めることで、滑らかな陰影を作る。
        // 正規化前の外積なので、大きい面ほど強く影響する。
        vertices[i0].norm = VAdd(vertices[i0].norm, faceNormal);
        vertices[i1].norm = VAdd(vertices[i1].norm, faceNormal);
        vertices[i2].norm = VAdd(vertices[i2].norm, faceNormal);
    }

    // 円周の始めと終わりは同じ場所の別頂点。向きもそろえて光の継ぎ目を目立ちにくくする。
    const float joinDistanceSquared = radius * radius * 0.00000001f;

    for (int y = 0; y <= LAT; ++y)
    {
        int first = y * (LON + 1);
        int last = first + LON;

        VECTOR difference = VSub(vertices[first].pos, vertices[last].pos);

        // 離れている頂点の法線は統合しない
        if (VSquareSize(difference) <= joinDistanceSquared)
        {
            VECTOR sum = VAdd(vertices[first].norm, vertices[last].norm);

            vertices[first].norm = sum;
            vertices[last].norm = sum;
        }
    }

    for (int pole = 0; pole < 2; ++pole)
    {
        int row = (pole == 0) ? 0 : LAT;
        int first = row * (LON + 1);

        bool samePosition = true;

        for (int x = 1; x < LON; ++x)
        {
            VECTOR difference = VSub(vertices[first + x].pos, vertices[first].pos);

            if (VSquareSize(difference) > joinDistanceSquared)
            {
                samePosition = false;
                break;
            }
        }

        if (samePosition)
        {
            VECTOR sum = VGet(0.0f, 0.0f, 0.0f);

            // 最後の頂点は先頭と重複するので数えない
            for (int x = 0; x < LON; ++x)
            {
                sum = VAdd(sum, vertices[first + x].norm);
            }

            for (int x = 0; x <= LON; ++x)
            {
                vertices[first + x].norm = sum;
            }
        }
    }

    // 法線を長さ1にそろえる
    for (size_t i = 0; i < vertices.size(); ++i)
    {
        if (VSquareSize(vertices[i].norm) > 0.000000000001f)
        {
            vertices[i].norm = VNorm(vertices[i].norm);
        }
        else
        {
            // 面がつぶれた場合の安全な予備値
            int row = static_cast<int>(i) / (LON + 1);

            vertices[i].norm = row > LAT / 2 ? VGet(0.0f, -1.0f, 0.0f) : VGet(0.0f, 1.0f, 0.0f);
        }
    }
}

void SlimeMesh::DrawFace(int graphHandle, const VECTOR& position, float radius, const VECTOR& faceForward)
{
    if (graphHandle < 0 || vertices.empty() || indices.empty())
    {
        return;
    }

    if (VSquareSize(faceForward) < 0.000001f)
    {
        return;
    }

    VECTOR forward = VNorm(faceForward);

    VECTOR worldUp = VGet(0.0f, 1.0f, 0.0f);
    VECTOR faceRight = CrossVector(worldUp, forward);

    if (VSquareSize(faceRight) < 0.000001f)
    {
        faceRight = VGet(1.0f, 0.0f, 0.0f);
    }
    else
    {
        faceRight = VNorm(faceRight);
    }

    VECTOR faceUp = CrossVector(forward, faceRight);

    if (VSquareSize(faceUp) < 0.000001f)
    {
        faceUp = VGet(0.0f, 1.0f, 0.0f);
    }
    else
    {
        faceUp = VNorm(faceUp);
    }

    // 顔全体の横幅と高さ
    const float faceWidth = radius * SlimeFaceConfig::kFaceWidthRate;
    const float faceHeight = radius * SlimeFaceConfig::kFaceHeightRate;

    // 顔の中心を少し下へ移動
    VECTOR faceCenter = VAdd(position, VScale(faceUp, radius * SlimeFaceConfig::kFaceCenterDownRate));

    std::vector<VERTEX3D> faceVertices;
    std::vector<unsigned short> faceIndices;

    faceVertices.reserve(indices.size());
    faceIndices.reserve(indices.size());

    // 三角形単位で調べる。顔の範囲に入っている三角形だけを顔メッシュへコピーする。
    for (size_t tri = 0; tri + 2 < indices.size(); tri += 3)
    {
        int srcIndex[3] =
        {
            static_cast<int>(indices[tri]),
            static_cast<int>(indices[tri + 1]),
            static_cast<int>(indices[tri + 2])
        };

        VERTEX3D temp[3];
        bool validTriangle = true;

        for (int corner = 0; corner < 3; ++corner)
        {
            const VERTEX3D& src = vertices[srcIndex[corner]];

            // カメラ側を向いていない面には顔を貼らない。
            float facing = VDot(src.norm, forward);

            if (facing < SlimeFaceConfig::kFacingThreshold)
            {
                validTriangle = false;
                break;
            }

            // 顔中心からこの頂点までの位置
            VECTOR offset = VSub(src.pos, faceCenter);

            // 表面を顔の「横・縦」方向へ投影し、0～1のUVへ変換する。
            float u = 0.5f + VDot(offset, faceRight) / faceWidth;
            float v = 0.5f - VDot(offset, faceUp) / faceHeight;

            // 顔画像の範囲外なら、この三角形は描かない。
            const float margin = SlimeFaceConfig::kUvMargin;

            if (u < -margin || u > 1.0f + margin || v < -margin || v > 1.0f + margin)
            {
                validTriangle = false;
                break;
            }

            temp[corner] = src;

            // 本体と完全に同じ位置だとちらつくため、法線方向へごくわずかに浮かせる。
            temp[corner].pos = VAdd(src.pos, VScale(src.norm, radius * SlimeFaceConfig::kFaceLiftRate));

            // 顔画像のUV
            temp[corner].u = ClampValue(u, 0.0f, 1.0f);
            temp[corner].v = ClampValue(v, 0.0f, 1.0f);
            temp[corner].su = 0.0f;
            temp[corner].sv = 0.0f;

            // 顔画像そのものの色を使いたいので白にする。PNG側の透明度がそのまま利用される。
            temp[corner].dif = GetColorU8(255, 255, 255, 255);
            temp[corner].spc = GetColorU8(0, 0, 0, 0);
        }

        if (!validTriangle)
        {
            continue;
        }

        unsigned short base = static_cast<unsigned short>(faceVertices.size());

        faceVertices.push_back(temp[0]);
        faceVertices.push_back(temp[1]);
        faceVertices.push_back(temp[2]);

        faceIndices.push_back(base);
        faceIndices.push_back(static_cast<unsigned short>(base + 1));
        faceIndices.push_back(static_cast<unsigned short>(base + 2));
    }

    if (faceVertices.empty())
    {
        return;
    }

    // 透明PNGを通常のアルファブレンドで描く。
    DxLib::SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    DrawPolygonIndexed3D(faceVertices.data(), static_cast<int>(faceVertices.size()), faceIndices.data(), static_cast<int>(faceIndices.size() / 3), graphHandle, TRUE);
    DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void SlimeMesh::Draw()
{
    if (vertices.empty() || indices.empty())
    {
        return;
    }

    // DX_NONE_GRAPHは画像なし。TRUEは透過を有効にする指定。
    DrawPolygonIndexed3D(vertices.data(), static_cast<int>(vertices.size()), indices.data(), static_cast<int>(indices.size() / 3), DX_NONE_GRAPH, TRUE);
}
