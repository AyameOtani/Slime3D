#include "SlimeModel.h"
#include "stage.h"
#include <cmath>
#include <cfloat>
#include <map>
#include <tuple>
#include <deque>


// 床判定の追加　大谷
namespace SlimeRaySettings
{
    // 床判定する位置の広さ
    constexpr float CheckRadiusRate = 0.75f;

    // 床判定の下限・上限
    constexpr float FloorCheckBelow = 1.65f;
    constexpr float FloorCheckAbove = 0.25f;
}


namespace
{
    // 横幅。大きくすると横に広がる
    const float BODY_WIDTH = 1.20f;

    // 高さ。小さくすると平たいスライムになる
    const float BODY_HEIGHT = 1.75f;

    // 赤ちゃんの基準サイズ。以前の0.48から0.22へ縮小。
    const float BABY_SCALE = 0.22f;

    // 下は濃く、頭は明るい色にする。色は生成時に各頂点へ保存する。
    COLOR_U8 SlimeColor(int color, float heightRate)
    {
        const int bottom[3][3] = { {205, 30, 55}, {25, 65, 220}, {10, 165, 90} };
        const int top[3][3] = { {255, 115, 70}, {40, 205, 255}, {190, 240, 45} };
        int c = color >= 1 && color <= 3 ? color - 1 : 0;
        float t = heightRate < 0.0f ? 0.0f : (heightRate > 1.0f ? 1.0f : heightRate);

        return GetColorU8(static_cast<int>(bottom[c][0] + (top[c][0] - bottom[c][0]) * t), static_cast<int>(bottom[c][1] + (top[c][1] - bottom[c][1]) * t), static_cast<int>(bottom[c][2] + (top[c][2] - bottom[c][2]) * t), 255);
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

    const float BABY_WIDTH = 1.18f;
    const float BABY_HEIGHT = 1.10f;

    // 半楕円の丸いドーム。底面を閉じ、頭には尖りを作らない。
    VECTOR BabyShape(float v, float phi)
    {
        float ring, height;
        if (v <= 0.90f)
        {
            float angle = v / 0.90f * DX_PI_F * 0.5f;
            ring = BABY_WIDTH * sinf(angle);
            height = BABY_HEIGHT * cosf(angle);
        }
        else
        {
            ring = BABY_WIDTH * (1.0f - v) / 0.10f;
            height = 0.0f;
        }
        return VGet(cosf(phi) * ring, -1.0f + height, sinf(phi) * ring);
    }

    // 分裂演出の長さ（更新回数）
    const float SPLIT_FRAMES = 48.0f;

    // 上半身の揺れ
    const float SWAY_X = 0.08f;
    const float SWAY_Z = 0.025f;

    // 表面の小さな膨らみ
    const float RIPPLE_POWER = 0.012f;

    // ベクトルの外積
    VECTOR CrossVector(const VECTOR& a, const VECTOR& b)
    {
        return VGet(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }

    // 頂点位置の補間
    VECTOR LerpVector(const VECTOR& from, const VECTOR& to, float rate)
    {
        return VAdd(VScale(from, 1.0f - rate), VScale(to, rate));
    }
}

// 赤ちゃんとペンキはこのcpp内で管理するため、SlimeModel.hの変更は不要。
namespace
{
    const int MAX_BABIES = 200;
    const int BATCH_BABIES = 120;
    const int BABY_LAT = 8;
    const int BABY_LON = 12;
    const float BABY_SPEED = 0.025f; // 1回のUpdateで進む距離（60fps想定）
    const int MAX_WATER_DROPS = 900; // Mキー破裂時の水滴上限
    const int MAX_BURST_STAINS = 800; // 破裂跡。上限を超えたら古い跡から消す
    const float PAINT_CELL = 0.14f;
    const int MAX_PAINT_CELLS = 12000; // 塗装データを無制限に増やさない

    struct WalkingBaby
    {
        VECTOR foot;                 // 地面に接する位置
        VECTOR groundNormal = VGet(0, 1, 0);
        float angle, speed, phase;
        float facing = 0.0f; // 最後に実際に進めた方向
        float jump = 0.0f, jumpSpeed = 0.0f;
        int turnTimer = 0, jumpTimer = 0, age = 0;
        int color = 1;
        VECTOR lastPaint;

        // Mキーで破裂させる演出用
        bool bursting = false;
        int burstFrame = 0;
    };
    // 子供スライムが水風船のように破裂した時の水滴。
    struct WaterDrop
    {
        VECTOR pos;
        VECTOR prevPos;
        VECTOR vel;
        int color = 1;
        int age = 0;
        int life = 36;
        float size = 0.035f;
    };

    struct SplashRing
    {
        VECTOR center;
        VECTOR normal = VGet(0, 1, 0);
        int color = 1;
        int age = 0;
        int life = 18;
    };

    // Mキーで破裂したあと、地面に残り続けるPNGの跡。
    // ageは「広がるアニメーション」にだけ使用し、寿命では消さない。
    struct BurstStain
    {
        VECTOR center;
        VECTOR normal = VGet(0, 1, 0);
        int color = 1;          // 1=赤(r.png), 2=青(b.png), 3=緑(g.png)
        int age = 0;
        float radius = 0.40f;   // 最終的な大きさ
        float rotation = 0.0f;  // 地面上での回転
        float stretchX = 1.0f;  // 横方向のつぶれ・伸び
        float stretchZ = 1.0f;
    };

    struct PaintCell
    {
        VECTOR center;
        int slot = 0;
        int bits = 0;
        int visits = 0;
    };
    struct BabyBatch
    {
        std::vector<VERTEX3D> vertices;
        std::vector<unsigned short> indices;
    };
    struct BabyWorld
    {
        std::vector<WalkingBaby> babies;
        std::vector<WaterDrop> waterDrops;
        std::vector<SplashRing> splashRings;
        std::vector<BurstStain> burstStains;
        std::map<std::tuple<int, int, int>, PaintCell> paint;
        std::deque<std::tuple<int, int, int>> paintOrder;
        std::vector<BabyBatch> paintBatches;
        std::vector<unsigned short> meshIndices;
        std::vector<VERTEX3D> bodyTemplate[3];
        std::vector<VERTEX3D> eyeTemplate;
        std::vector<unsigned short> eyeIndices;
        BabyBatch bodyBatch, eyeBatch;
        int frame = 0, spawnTimer = 0;
        bool oldX = false;
        bool oldM = false;
    };
    // 元のヘッダーを維持するため、所有者ごとに状態を分離する。
    // Initializeで当該所有者の前回の状態を破棄する。
    std::map<const SlimeModel*, BabyWorld> babyWorlds;

    float BabyRandom(float low, float high)
    {
        return low + (high - low) * (GetRand(10000) / 10000.0f);
    }

    unsigned int WaterColor(int color)
    {
        // 元のスライム色より少し明るくして、水っぽいしぶきにする。
        if (color == 2) return GetColor(70, 170, 255);
        if (color == 3) return GetColor(90, 245, 150);
        return GetColor(255, 105, 125);
    }

    // -----------------------------------------------------------------
    // 破裂跡PNG
    // color 1=赤, 2=青, 3=緑
    // ヘッダーを変更しないため、このcpp内で遅延ロードする。
    // -----------------------------------------------------------------
    int GetBurstStainHandle(int color)
    {
        static int handles[3] = { -2, -2, -2 }; // -2=まだLoadGraphしていない
        const char* files[3] =
        {
            "Resource\\Slime\\r.png",
            "Resource\\Slime\\b.png",
            "Resource\\Slime\\g.png"
        };

        int index = color - 1;
        if (index < 0 || index >= 3) return -1;

        if (handles[index] == -2)
        {
            handles[index] = LoadGraph(files[index]);
        }
        return handles[index];
    }

    void AddBurstStain(BabyWorld& world, const BurstStain& stain)
    {
        // 跡は基本的に残す。ただし無制限に増えると重くなるので上限だけ設ける。
        if (static_cast<int>(world.burstStains.size()) >= MAX_BURST_STAINS)
        {
            world.burstStains.erase(world.burstStains.begin());
        }
        world.burstStains.push_back(stain);
    }

    void SpawnBurstStains(BabyWorld& world, const WalkingBaby& baby, float babyRadius)
    {
        VECTOR normal = baby.groundNormal;
        if (VSquareSize(normal) < 0.000001f) normal = VGet(0.0f, 1.0f, 0.0f);
        else normal = VNorm(normal);

        // 床と完全に同じ高さだとちらつくので、法線方向へほんの少し浮かせる。
        VECTOR baseCenter = VAdd(baby.foot, VScale(normal, 0.012f));

        // 中央の大きな跡。
        BurstStain mainStain;
        mainStain.center = baseCenter;
        mainStain.normal = normal;
        mainStain.color = baby.color;
        mainStain.radius = babyRadius * BabyRandom(1.85f, 2.55f);
        mainStain.rotation = BabyRandom(0.0f, DX_PI_F * 2.0f);
        mainStain.stretchX = BabyRandom(0.90f, 1.25f);
        mainStain.stretchZ = BabyRandom(0.90f, 1.25f);
        AddBurstStain(world, mainStain);

        // 血しぶきのように見える、小さな飛び散り跡を3つ追加。
        // 同じPNGを縮小・回転して使うため、追加画像は不要。
        for (int i = 0; i < 3; ++i)
        {
            float angle = BabyRandom(0.0f, DX_PI_F * 2.0f);
            float distance = mainStain.radius * BabyRandom(0.65f, 1.35f);

            // 地面の法線に沿う2本の軸を作る。
            VECTOR reference = fabsf(normal.y) < 0.95f
                ? VGet(0.0f, 1.0f, 0.0f)
                : VGet(0.0f, 0.0f, 1.0f);
            VECTOR axisX = CrossVector(reference, normal);
            if (VSquareSize(axisX) < 0.000001f) axisX = VGet(1.0f, 0.0f, 0.0f);
            else axisX = VNorm(axisX);
            VECTOR axisZ = CrossVector(normal, axisX);
            if (VSquareSize(axisZ) < 0.000001f) axisZ = VGet(0.0f, 0.0f, 1.0f);
            else axisZ = VNorm(axisZ);

            BurstStain smallStain;

            smallStain.center = VAdd(
                baseCenter,
                VAdd(
                    VScale(axisX, cosf(angle) * distance),
                    VScale(axisZ, sinf(angle) * distance)
                )
            );

            smallStain.normal = normal;
            smallStain.color = baby.color;
            smallStain.radius = babyRadius * BabyRandom(0.28f, 0.58f);
            smallStain.rotation = BabyRandom(0.0f, DX_PI_F * 2.0f);
            smallStain.stretchX = BabyRandom(0.65f, 1.35f);
            smallStain.stretchZ = BabyRandom(0.65f, 1.35f);

            AddBurstStain(world, smallStain);
        }
    }

    void SpawnWaterBurst(BabyWorld& world, const WalkingBaby& baby, float babyRadius)
    {
        // 1体から6個。200体同時でも上限900個で止める。
        const int DROP_COUNT = 6;
        VECTOR center = VAdd(baby.foot, VScale(baby.groundNormal, babyRadius * 0.75f));

        for (int i = 0; i < DROP_COUNT && static_cast<int>(world.waterDrops.size()) < MAX_WATER_DROPS; ++i)
        {
            float a = BabyRandom(0.0f, DX_PI_F * 2.0f);
            float side = BabyRandom(0.045f, 0.105f);
            float up = BabyRandom(0.055f, 0.135f);

            WaterDrop drop;
            drop.pos = center;
            drop.prevPos = center;
            drop.vel = VGet(cosf(a) * side, up, sinf(a) * side);
            drop.color = baby.color;
            drop.life = 28 + GetRand(18);
            drop.size = BabyRandom(babyRadius * 0.08f, babyRadius * 0.18f);
            world.waterDrops.push_back(drop);
        }

        SplashRing ring;
        ring.center = VAdd(baby.foot, VScale(baby.groundNormal, 0.012f));
        ring.normal = baby.groundNormal;
        ring.color = baby.color;
        world.splashRings.push_back(ring);

        // 破裂した地点をステージの実際の三角形にも赤・青・緑で塗る。
        // 子供の歩行や破裂演出はそのまま利用する。
        if (Stage* stage = Stage::Active())
        {
            const VECTOR contact = VAdd(baby.foot, VScale(baby.groundNormal, 0.025f));
            stage->Paint(contact, babyRadius * 0.25f, babyRadius * 2.0f, baby.color);
        }

        // 既存のr.png / b.png / g.pngの液体跡も残す。
        SpawnBurstStains(world, baby, babyRadius);
    }

    void UpdateWaterBurst(BabyWorld& world)
    {
        for (WaterDrop& drop : world.waterDrops)
        {
            ++drop.age;
            drop.prevPos = drop.pos;
            drop.vel.y -= 0.0065f;
            drop.pos = VAdd(drop.pos, drop.vel);
            drop.vel.x *= 0.992f;
            drop.vel.z *= 0.992f;
        }

        for (int i = static_cast<int>(world.waterDrops.size()) - 1; i >= 0; --i)
        {
            if (world.waterDrops[i].age >= world.waterDrops[i].life)
                world.waterDrops.erase(world.waterDrops.begin() + i);
        }

        for (SplashRing& ring : world.splashRings) ++ring.age;
        for (int i = static_cast<int>(world.splashRings.size()) - 1; i >= 0; --i)
        {
            if (world.splashRings[i].age >= world.splashRings[i].life)
                world.splashRings.erase(world.splashRings.begin() + i);
        }

        // 破裂跡は消さない。最初の18フレームだけ少しずつ広げる。
        for (BurstStain& stain : world.burstStains)
        {
            if (stain.age < 18) ++stain.age;
        }
    }

    void DrawBurstStains(BabyWorld& world)
    {
        if (world.burstStains.empty()) return;

        // PNGの透明部分を利用して、不規則な液体跡だけを地面に残す。
        DxLib::SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);

        static const unsigned short quadIndices[6] = { 0, 1, 2, 0, 2, 3 };

        for (const BurstStain& stain : world.burstStains)
        {
            int graph = GetBurstStainHandle(stain.color);
            if (graph < 0) continue;

            VECTOR normal = stain.normal;
            if (VSquareSize(normal) < 0.000001f) normal = VGet(0.0f, 1.0f, 0.0f);
            else normal = VNorm(normal);

            // 地面の傾きに合わせた2本の軸を作る。
            VECTOR reference = fabsf(normal.y) < 0.95f
                ? VGet(0.0f, 1.0f, 0.0f)
                : VGet(0.0f, 0.0f, 1.0f);

            VECTOR tangent = CrossVector(reference, normal);
            if (VSquareSize(tangent) < 0.000001f) tangent = VGet(1.0f, 0.0f, 0.0f);
            else tangent = VNorm(tangent);

            VECTOR bitangent = CrossVector(normal, tangent);
            if (VSquareSize(bitangent) < 0.000001f) bitangent = VGet(0.0f, 0.0f, 1.0f);
            else bitangent = VNorm(bitangent);

            // PNGを毎回同じ向きに見せないよう、地面上で回転させる。
            float c = cosf(stain.rotation);
            float s = sinf(stain.rotation);
            VECTOR axisX = VAdd(VScale(tangent, c), VScale(bitangent, s));
            VECTOR axisZ = VAdd(VScale(tangent, -s), VScale(bitangent, c));

            // 0→18フレームで「じわっ」と広がる。
            float t = stain.age / 18.0f;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            // smoothstep。急に大きくせず、最後にゆっくり止まる。
            t = t * t * (3.0f - 2.0f * t);
            float spread = 0.18f + 0.82f * t;

            float halfX = stain.radius * stain.stretchX * spread;
            float halfZ = stain.radius * stain.stretchZ * spread;

            VERTEX3D v[4] = {};
            v[0].pos = VAdd(stain.center, VAdd(VScale(axisX, -halfX), VScale(axisZ, -halfZ)));
            v[1].pos = VAdd(stain.center, VAdd(VScale(axisX, halfX), VScale(axisZ, -halfZ)));
            v[2].pos = VAdd(stain.center, VAdd(VScale(axisX, halfX), VScale(axisZ, halfZ)));
            v[3].pos = VAdd(stain.center, VAdd(VScale(axisX, -halfX), VScale(axisZ, halfZ)));

            const float uv[4][2] =
            {
                {0.0f, 1.0f},
                {1.0f, 1.0f},
                {1.0f, 0.0f},
                {0.0f, 0.0f}
            };

            for (int i = 0; i < 4; ++i)
            {
                v[i].norm = normal;
                v[i].dif = GetColorU8(255, 255, 255, 255);
                v[i].spc = GetColorU8(0, 0, 0, 0);
                v[i].u = uv[i][0];
                v[i].v = uv[i][1];
            }

            DrawPolygonIndexed3D(v, 4, quadIndices, 2, graph, TRUE);
        }

        DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    void DrawWaterBurst(BabyWorld& world)
    {
        // 水滴。球＋短い線を描き、飛んでいる方向へ伸びて見えるようにする。
        DxLib::SetDrawBlendMode(DX_BLENDMODE_ALPHA, 215);
        for (const WaterDrop& drop : world.waterDrops)
        {
            float remain = 1.0f - static_cast<float>(drop.age) / drop.life;
            float size = drop.size * (0.45f + 0.55f * remain);
            unsigned int c = WaterColor(drop.color);
            DrawLine3D(drop.prevPos, drop.pos, c);
            DrawSphere3D(drop.pos, size, 5, c, GetColor(255, 255, 255), TRUE);
        }

        // 地面に広がる一瞬の波紋。
        for (const SplashRing& ring : world.splashRings)
        {
            float t = static_cast<float>(ring.age) / ring.life;
            float rad = 0.05f + t * 0.42f;
            const int SEG = 14;
            unsigned int c = WaterColor(ring.color);
            for (int i = 0; i < SEG; ++i)
            {
                float a0 = DX_PI_F * 2.0f * i / SEG;
                float a1 = DX_PI_F * 2.0f * (i + 1) / SEG;
                VECTOR p0 = VAdd(ring.center, VGet(cosf(a0) * rad, 0.0f, sinf(a0) * rad));
                VECTOR p1 = VAdd(ring.center, VGet(cosf(a1) * rad, 0.0f, sinf(a1) * rad));
                DrawLine3D(p0, p1, c);
            }
        }
        DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 床の端から体がはみ出さず、壁にも重ならない移動先を調べる。
    bool BabyFloor(float x, float z, float currentY, float r, const std::vector<BoxCollider>& boxes, float& floorY)
    {
        if (Stage::Active()) return Stage::Active()->BabyCanStand(x, z, currentY, r, floorY);
        bool found = false;
        floorY = -FLT_MAX;
        for (const BoxCollider& box : boxes)
        {
            if (x - r < box.minPos.x || x + r > box.maxPos.x || z - r < box.minPos.z || z + r > box.maxPos.z) continue;
            const float y = box.maxPos.y;
            // 低い段差は越える。大きな崖には進まない。
            if (y > currentY + 0.12f || y < currentY - 0.35f) continue;
            if (!found || y > floorY) { floorY = y; found = true; }
        }
        if (!found) return false;
        for (const BoxCollider& box : boxes)
        {
            if (box.maxPos.y <= floorY + 0.001f || box.minPos.y >= floorY + r * 1.1f) continue;
            if (x + r > box.minPos.x && x - r < box.maxPos.x && z + r > box.minPos.z && z - r < box.maxPos.z) return false;
        }
        return true;
    }

    std::tuple<int, int, int> PaintKey(const VECTOR& p)
    {
        return std::make_tuple(static_cast<int>(floorf(p.x / PAINT_CELL)), static_cast<int>(floorf(p.z / PAINT_CELL)), static_cast<int>(floorf(p.y * 100.0f + 0.5f)));
    }

    // 未塗装の方向を優先。同じ場所だけを往復しにくくする。
    float ChooseBabyDirection(const WalkingBaby& baby, float r, const std::vector<BoxCollider>& boxes, const BabyWorld& world)
    {
        float best = baby.angle + DX_PI_F;
        float bestScore = -FLT_MAX;
        for (int i = 0; i < 10; ++i)
        {
            float a = baby.angle + (i - 4.5f) * (2.0f * DX_PI_F / 10.0f);
            VECTOR p = baby.foot;
            float score = BabyRandom(0.0f, 2.0f);
            bool valid = true;
            for (int step = 1; step <= 5; ++step)
            {
                float y;
                p.x = baby.foot.x + cosf(a) * step * 0.12f;
                p.z = baby.foot.z + sinf(a) * step * 0.12f;
                if (!BabyFloor(p.x, p.z, p.y, r, boxes, y)) { valid = false; break; }
                p.y = y;
                auto it = world.paint.find(PaintKey(p));
                score += it == world.paint.end() ? 4.0f : 1.0f / (1.0f + it->second.visits);
            }
            if (valid && score > bestScore) { bestScore = score; best = a; }
        }
        return best;
    }

    // 1セル10角形、5000セルずつ。最大55000頂点なので16bit番号で安全。
    void UpdatePaintSlot(BabyWorld& world, const PaintCell& cell)
    {
        const int SEG = 10, CELLS_PER_BATCH = 5000, STRIDE = SEG + 1;
        int batchId = cell.slot / CELLS_PER_BATCH;
        int offset = cell.slot % CELLS_PER_BATCH;
        if (static_cast<int>(world.paintBatches.size()) <= batchId)
        {
            world.paintBatches.resize(batchId + 1);
            world.paintBatches.back().vertices.reserve(CELLS_PER_BATCH * STRIDE);
            world.paintBatches.back().indices.reserve(CELLS_PER_BATCH * SEG * 3);
        }
        BabyBatch& batch = world.paintBatches[batchId];
        int base = offset * STRIDE;
        if (static_cast<int>(batch.vertices.size()) < base + STRIDE)
        {
            batch.vertices.resize(base + STRIDE);
            for (int i = 0; i < SEG; ++i)
            {
                batch.indices.push_back(static_cast<unsigned short>(base));
                batch.indices.push_back(static_cast<unsigned short>(base + i + 1));
                batch.indices.push_back(static_cast<unsigned short>(base + (i + 1) % SEG + 1));
            }
        }
        // 円周のsin/cosも一度だけ計算する。
        static std::vector<VECTOR> circle;
        if (circle.empty())
        {
            circle.push_back(VGet(0.0f, 0.0f, 0.0f));
            for (int i = 0; i < SEG; ++i)
            {
                float a = i * DX_PI_F * 2.0f / SEG;
                circle.push_back(VGet(cosf(a) * PAINT_CELL * 0.8f, 0.0f, sinf(a) * PAINT_CELL * 0.8f));
            }
        }
        COLOR_U8 color = GetColorU8((cell.bits & 1) ? 245 : 25, (cell.bits & 4) ? 225 : 25, (cell.bits & 2) ? 245 : 25, 255);
        for (int i = 0; i < STRIDE; ++i)
        {
            VERTEX3D& v = batch.vertices[base + i];
            v = {};
            v.pos = VAdd(cell.center, circle[i]); v.pos.y += 0.008f;
            v.norm = VGet(0.0f, 1.0f, 0.0f); v.dif = color;
        }
    }

    //子供スライムが地面を塗りつぶす
    void StampBabyPaint(BabyWorld& world, const WalkingBaby& baby, float r, const std::vector<BoxCollider>& boxes)
    {

        //if (Stage* stage = Stage::Active())
        //{
        //    // 実際の斜面へ塗る。移動方向選び用の訪問履歴だけスライム側に残す。
        //    stage->Paint(VAdd(baby.foot, VScale(baby.groundNormal, 0.02f)), r * 0.3f, r * 0.95f, baby.color);
        //    auto key = PaintKey(baby.foot);
        //    if (world.paint.find(key) == world.paint.end())
        //    {
        //        if (world.paint.size() >= MAX_PAINT_CELLS)
        //        {
        //            world.paint.erase(world.paintOrder.front()); world.paintOrder.pop_front();
        //        }
        //        world.paintOrder.push_back(key);
        //        PaintCell cell; cell.center = baby.foot; world.paint.emplace(key, cell);
        //    }
        //    PaintCell& cell = world.paint[key];
        //    if (cell.visits < 1000) ++cell.visits;
        //    cell.bits |= 1 << (baby.color - 1);
        //    return;
        //}
        //// 小さな円を重ねるので、四角いマスの縁が見えにくい。
        //const float brush = r * 0.95f;
        //int cells = static_cast<int>(ceilf(brush / PAINT_CELL));
        //auto origin = PaintKey(baby.foot);
        //for (int dz = -cells; dz <= cells; ++dz)
        //    for (int dx = -cells; dx <= cells; ++dx)
        //    {
        //        int gx = std::get<0>(origin) + dx, gz = std::get<1>(origin) + dz;
        //        VECTOR p = VGet((gx + 0.5f) * PAINT_CELL, baby.foot.y, (gz + 0.5f) * PAINT_CELL);
        //        float px = p.x - baby.foot.x, pz = p.z - baby.foot.z;
        //        if (px * px + pz * pz > brush * brush) continue;
        //        float y;
        //        // ペンキが床の外へ浮かないよう、円全体が乗る場所だけ塗る。
        //        if (!BabyFloor(p.x, p.z, p.y, PAINT_CELL * 0.80f, boxes, y) || fabsf(y - p.y) > 0.02f) continue;
        //        p.y = y;
        //        auto key = PaintKey(p);
        //        auto it = world.paint.find(key);
        //        if (it == world.paint.end())
        //        {
        //            int slot = static_cast<int>(world.paint.size());
        //            // 上限では古い塗装の描画領域を再利用する。
        //            if (world.paint.size() >= MAX_PAINT_CELLS)
        //            {
        //                slot = world.paint.find(world.paintOrder.front())->second.slot;
        //                world.paint.erase(world.paintOrder.front());
        //                world.paintOrder.pop_front();
        //            }
        //            world.paintOrder.push_back(key);
        //            PaintCell cell; cell.center = p; cell.slot = slot;
        //            it = world.paint.emplace(key, cell).first;
        //        }
        //        int bits = it->second.bits | (1 << (baby.color - 1));
        //        if (bits != it->second.bits)
        //        {
        //            it->second.bits = bits;
        //            UpdatePaintSlot(world, it->second);
        //        }
        //        if (it->second.visits < 1000) ++it->second.visits;
        //    }
    }

    void UpdateWalkingBabies(BabyWorld& world, float r, const std::vector<BoxCollider>& boxes)
    {
        ++world.frame;
        for (WalkingBaby& baby : world.babies)
        {
            ++baby.age;
            baby.phase += 0.16f;

            // Mキーで破裂中は歩かせない。
            // 4フレーム目に本体メッシュが裂け始め、その瞬間に水滴も発生させる。
            if (baby.bursting)
            {
                ++baby.burstFrame;
                baby.speed = 0.0f;
                baby.jumpSpeed = 0.0f;
                baby.jump *= 0.55f;

                if (baby.burstFrame == 4)
                {
                    SpawnWaterBurst(world, baby, r / BABY_WIDTH);
                }
                continue;
            }

            if (--baby.turnTimer <= 0)
            {
                baby.angle = ChooseBabyDirection(baby, r, boxes, world);
                baby.turnTimer = 30 + GetRand(70);
            }
            float x = baby.foot.x + cosf(baby.angle) * baby.speed;
            float z = baby.foot.z + sinf(baby.angle) * baby.speed;
            float y;
            if (BabyFloor(x, z, baby.foot.y, r, boxes, y))
            {
                baby.foot = VGet(x, y, z);
                if (Stage::Active()) { Stage::Active()->FindFloor(x, z, y - 0.01f, y + 0.01f, y, &baby.groundNormal); }
                baby.facing = baby.angle; // カメラではなく実際の移動方向を向く
            }
            else { baby.angle += BabyRandom(1.4f, 4.8f); baby.turnTimer = 0; }

            if (--baby.jumpTimer <= 0 && baby.jump <= 0.0f)
            {
                baby.jumpSpeed = BabyRandom(0.035f, 0.055f);
                baby.jumpTimer = 120 + GetRand(240);
            }
            if (baby.jump > 0.0f || baby.jumpSpeed > 0.0f)
            {
                baby.jumpSpeed -= 0.006f;
                baby.jump += baby.jumpSpeed;
                if (baby.jump < 0.0f) { baby.jump = 0.0f; baby.jumpSpeed = 0.0f; }
            }
            if (baby.jump <= 0.0f &&
                (baby.age == 1 || VSquareSize(VSub(baby.foot, baby.lastPaint)) >= 0.0064f))
            {
                StampBabyPaint(world, baby, r, boxes);
                baby.lastPaint = baby.foot;
            }
        }

        // 本体とは別に、水滴と波紋はしばらく残す。
        UpdateWaterBurst(world);

        // 破裂して本体が消えた子供を削除する。
        // 後ろから消すことで vector の番号ずれを避ける。
        for (int i = static_cast<int>(world.babies.size()) - 1; i >= 0; --i)
        {
            if (world.babies[i].bursting && world.babies[i].burstFrame >= 20)
            {
                world.babies.erase(world.babies.begin() + i);
            }
        }
    }

    void DrawBabyPaint(BabyWorld& world)
    {
        // 塗装データはStamp時に変更箇所だけ更新済み。全件再構築しない。
        for (BabyBatch& batch : world.paintBatches)
        {
            DrawPolygonIndexed3D(batch.vertices.data(), static_cast<int>(batch.vertices.size()), batch.indices.data(), static_cast<int>(batch.indices.size() / 3), DX_NONE_GRAPH, TRUE);
        }
    }

}

SlimeModel::SlimeModel()
    : selectedColor(1),
    oldSpace(false),
    currentFace(0),
    faceDirectionX(1.0f),
    faceTurnX(1.0f)
{
    ResetBody();
    for (int i = 0; i < 5; ++i) faceHandles[i] = -1;
    oldZ = false;

    for (int i = 0; i < 3; ++i)
    {
        oldNumberKeys[i] = false;
    }
}

float SlimeModel::Clamp(float value, float minValue, float maxValue)
{
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;

    return value;
}

void SlimeModel::Initialize()
{
    ResetBody();
    CreateMesh();
    babyWorlds[this] = BabyWorld();
    babyWorlds[this].babies.reserve(MAX_BABIES);

    selectedColor = 1;

    oldSpace = false;

    // 5種類の表情画像。
    const char* faceFiles[5] = {
        "Resource\\Slime\\face_a.png", "Resource\\Slime\\face_b.png", "Resource\\Slime\\face_c.png",
        "Resource\\Slime\\face_d.png", "Resource\\Slime\\face_e.png"
    };
    for (int i = 0; i < 5; ++i)
    {
        if (faceHandles[i] >= 0) DeleteGraph(faceHandles[i]);
        faceHandles[i] = LoadGraph(faceFiles[i]);
    }
    currentFace = 0;
    // aがあればaを初期表示。なければ最初に読めた画像を使う。
    for (int i = 0; i < 5; ++i)
    {
        if (faceHandles[i] >= 0) { currentFace = i + 1; break; }
    }

    // 最初は右向き
    faceDirectionX = 1.0f;
    faceTurnX = 1.0f;

    for (int i = 0; i < 3; ++i)
    {
        oldNumberKeys[i] = false;
    }

    // 分裂したスライムも初期化
    splitSlimes.clear();
    oldZ = false;
}

void SlimeModel::ResetBody()
{
    // positionは本体の基準位置、velocityは更新1回で動く量。Yのプラスは上。
    position = VGet(0.0f, 1.0f, -1.5f);
    velocity = VGet(0.0f, 0.0f, 0.0f);

    // radiusは球形の当たり判定の半径。見た目の大きさにも使う。
    // groundedがtrueなら、当たり判定で地面に乗っていると判断された状態。
    radius = 0.95f;
    grounded = false;

    // 縦の倍率。1は通常、1未満はつぶれる、1より大きいと伸びる。
    // squashVelocityはその倍率が変わる勢い。ばねのような揺れに使う。
    squashScale = 1.0f;
    squashVelocity = 0.0f;

    // Phaseは揺れの周期のどこにいるか、Powerは揺れの強さ。
    wobblePhase = 0.0f;
    wobblePower = 0.0f;

    // droopは空中で下に垂れる量。dentは壁側のへこみ量。
    droop = 0.0f;

    dent = 0.0f;
    dentVelocity = 0.0f;

    dentDirection = VGet(0.0f, 0.0f, 0.0f);
}

void SlimeModel::CreateMesh()
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


void SlimeModel::SelectColorByKeyboard()
{
    const int keys[3] = { KEY_INPUT_1, KEY_INPUT_2, KEY_INPUT_3 };

    for (int i = 0; i < 3; ++i)
    {
        bool pressed = CheckHitKey(keys[i]) != 0;

        if (pressed && !oldNumberKeys[i]) { selectedColor = i + 1; }
        oldNumberKeys[i] = pressed;
    }
}

// 表情切り替え
void SlimeModel::UpdateFace()
{
    const int keys[5] = { KEY_INPUT_Y, KEY_INPUT_U, KEY_INPUT_I, KEY_INPUT_O, KEY_INPUT_P };
    for (int i = 0; i < 5; ++i)
    {
        // 読み込めない画像を選んだ場合は、現在の顔を維持する。
        if (CheckHitKey(keys[i]) && faceHandles[i] >= 0) currentFace = i + 1;
    }


    bool left = CheckHitKey(KEY_INPUT_A) != 0;
    bool right = CheckHitKey(KEY_INPUT_D) != 0;

    if (left && !right)
    {
        faceDirectionX = -1.0f;
    }
    else if (right && !left)
    {
        faceDirectionX = 1.0f;
    }

    faceTurnX += (faceDirectionX - faceTurnX) * 0.18f;
}

VECTOR SlimeModel::ClosestPoint(const VECTOR& p, const BoxCollider& box)
{
    return VGet(Clamp(p.x, box.minPos.x, box.maxPos.x), Clamp(p.y, box.minPos.y, box.maxPos.y), Clamp(p.z, box.minPos.z, box.maxPos.z));
}

void SlimeModel::ResolveCollision(const BoxCollider& box)
{
    VECTOR closest = ClosestPoint(position, box);
    VECTOR difference = VSub(position, closest);

    // 中心と箱の最近点の距離を2乗のまま比較すると、平方根の計算を省ける。
    float distanceSquared = VSquareSize(difference);

    if (distanceSquared > radius * radius)
    {
        return;
    }

    // normalは箱から本体を押し出す向き、penetrationはめり込んでいる深さ。
    VECTOR normal = VGet(0.0f, 1.0f, 0.0f);
    float penetration = 0.0f;

    if (distanceSquared > 0.000001f)
    {
        float distance = sqrtf(distanceSquared);

        normal = VScale(difference, 1.0f / distance);
        penetration = radius - distance;
    }
    else
    {
        float distances[6] =
        {
            position.x - box.minPos.x,
            box.maxPos.x - position.x,

            position.y - box.minPos.y,
            box.maxPos.y - position.y,

            position.z - box.minPos.z,
            box.maxPos.z - position.z
        };

        VECTOR normals[6] =
        {
            VGet(-1.0f,  0.0f,  0.0f),
            VGet(1.0f,  0.0f,  0.0f),
            VGet(0.0f, -1.0f,  0.0f),
            VGet(0.0f,  1.0f,  0.0f),
            VGet(0.0f,  0.0f, -1.0f),
            VGet(0.0f,  0.0f,  1.0f)
        };

        int nearest = 0;

        for (int i = 1; i < 6; ++i)
        {
            if (distances[i] < distances[nearest])
            {
                nearest = i;
            }
        }

        normal = normals[nearest];
        penetration = radius + distances[nearest];
    }

    // めり込みを解消
    position = VAdd(position, VScale(normal, penetration));

    // 押し出す向きに対する速度。負なら箱の内側へ進んでいる。
    // その成分だけ取り除き、壁に沿う方向の移動は残す。
    float velocityNormal = VDot(velocity, normal);
    float impactSpeed = 0.0f;

    if (velocityNormal < 0.0f)
    {
        impactSpeed = -velocityNormal;
        velocity = VSub(velocity, VScale(normal, velocityNormal));
    }

    // 床への着地
    // 押し出す方向が十分上向きなら着地とみなす。衝突が強いほどつぶれ・揺れを増やす。
    if (normal.y > 0.55f)
    {
        grounded = true;

        if (impactSpeed > 0.025f)
        {
            squashVelocity -= impactSpeed * 0.9f;
            wobblePower += impactSpeed * 3.0f;
        }
    }

    // 壁への接触
    // 押し出す方向がほぼ水平なら壁への接触とみなす。壁側へ向いた部分をへこませる。
    if (fabsf(normal.y) < 0.45f)
    {
        VECTOR wallDirection = VScale(normal, -1.0f);
        wallDirection.y = 0.0f;

        if (VSize(wallDirection) > 0.001f)
        {
            dentDirection = VNorm(wallDirection);
            float amount = Clamp(penetration * 0.7f + impactSpeed * 2.2f, 0.05f, 0.55f);

            if (amount > dent)
            {
                dent = amount;
            }

            wobblePower += impactSpeed * 2.0f;
        }
    }
}

bool SlimeModel::HasSupport(float x, float z, float centerY, float r, const std::vector<BoxCollider>& boxes) const
{
    if (Stage* stage = Stage::Active())
    {
        float floorY;
        return stage->FindFloor(x, z, centerY - r * 1.65f, centerY + r * 0.25f, floorY);
    }
    for (const BoxCollider& box : boxes)
    {
        if (x < box.minPos.x || x > box.maxPos.x || z < box.minPos.z || z > box.maxPos.z)
        {
            continue;
        }

        float topY = box.maxPos.y;

        if (topY > centerY + r * 0.25f)
        {
            continue;
        }

        if (centerY - topY > r * 1.65f)
        {
            continue;
        }

        return true;
    }

    return false;
}

float SlimeModel::GetSupportAmount(float x, float z, float centerY, float r, const std::vector<BoxCollider>& boxes) const
{
    // 調べたい点の周囲9か所を確認する。1点だけの判定より足場の端の変化を滑らかにする。
    const float offsets[9][2] =
    {
        { 0.0f,  0.0f },
        { 1.0f,  0.0f },
        {-1.0f,  0.0f },
        { 0.0f,  1.0f },
        { 0.0f, -1.0f },
        { 0.7f,  0.7f },
        {-0.7f,  0.7f },
        { 0.7f, -0.7f },
        {-0.7f, -0.7f }
    };

    int count = 0;
    float distance = r * 0.10f;

    for (int i = 0; i < 9; ++i)
    {
        if (HasSupport(x + offsets[i][0] * distance, z + offsets[i][1] * distance, centerY, r, boxes))
        {
            ++count;
        }
    }

    // 9点中9点なら1、0点なら0。これは9点で推定した割合で、厳密な接地面積ではない。
    return static_cast<float>(count) / 9.0f;
}

void SlimeModel::UpdateBody(const std::vector<BoxCollider>& boxes)
{
    // 前回の接地状態を入力処理用に覚える。今回の接地は、この後の当たり判定で調べ直す。
    bool wasGrounded = grounded;
    grounded = false;

    // 移動入力
    VECTOR input = VGet(0.0f, 0.0f, 0.0f);

    if (CheckHitKey(KEY_INPUT_W)) input.z += 1.0f;
    if (CheckHitKey(KEY_INPUT_S)) input.z -= 1.0f;
    if (CheckHitKey(KEY_INPUT_A)) input.x -= 1.0f;
    if (CheckHitKey(KEY_INPUT_D)) input.x += 1.0f;

    if (VSize(input) > 0.001f)
    {
        // 斜め入力は矢印が長くなるので、長さを1にそろえ、斜めだけ加速が強くなるのを防ぐ。
        input = VNorm(input);

        velocity.x += input.x * 0.020f;
        velocity.z += input.z * 0.020f;
    }

    // 摩擦
    // 速度に1未満を掛けて減速。地上は止まりやすく、空中は勢いを残す設定。
    float friction = wasGrounded ? 0.86f : 0.985f;

    velocity.x *= friction;
    velocity.z *= friction;

    // 最大速度
    float horizontalSpeed = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);

    const float maxSpeed = 0.16f;

    if (horizontalSpeed > maxSpeed)
    {
        // 速度の方向を保ち、長さだけを上限にそろえるための倍率。
        float rate = maxSpeed / horizontalSpeed;

        velocity.x *= rate;
        velocity.z *= rate;
    }

    // ジャンプ
    bool space = CheckHitKey(KEY_INPUT_SPACE) != 0;

    // Spaceを押した瞬間、かつ前回地面にいた場合だけジャンプ。
    if (space && !oldSpace && wasGrounded)
    {
        velocity.y = 0.25f;

        squashVelocity -= 0.08f;
        wobblePower += 0.35f;
    }

    oldSpace = space;

    // 重力
    // 毎回下向きの速度を増やす。下の制限処理で落下速度を最大0.45に抑える。
    velocity.y -= 0.012f;

    if (velocity.y < -0.45f)
    {
        velocity.y = -0.45f;
    }




    // ★★★ 5本のRay判定を追加しました。　大谷 ★★
    if (Stage* stage = Stage::Active())
    {
        const float nextX = position.x + velocity.x;
        const float nextZ = position.z + velocity.z;

        // 中央、前後、左右
        const float offsets[5][2] =
        {
            { 0.0f,  0.0f },
            { 0.0f,  1.0f },
            { 0.0f, -1.0f },
            {-1.0f,  0.0f },
            { 1.0f,  0.0f }
        };

        const float checkRadius =
            radius * SlimeRaySettings::CheckRadiusRate;

        bool canMove = true;

        for (int i = 0; i < 5; ++i)
        {
            float rayX = nextX + offsets[i][0] * checkRadius;
            float rayZ = nextZ + offsets[i][1] * checkRadius;
            float floorY = 0.0f;

            if (!stage->FindFloor(
                rayX,
                rayZ,
                position.y - radius * SlimeRaySettings::FloorCheckBelow,
                position.y + radius * SlimeRaySettings::FloorCheckAbove,
                floorY))
            {
                canMove = false;
                break;
            }
        }

        // 5本のうち1本でも床がなければ水平移動を止める
        if (!canMove)
        {
            velocity.x = 0.0f;
            velocity.z = 0.0f;
        }
    }
    // ★★★ ここまで追加しました　大谷 ★★★




    // 現在位置＋移動量＝次の位置。速度を変えた後に位置を更新する。
    position = VAdd(position, velocity);

    // MQOの実際の三角形との衝突。斜面は斜面の法線で押し戻す。
    if (Stage* stage = Stage::Active())
    {
        float impact = 0, wallDepth = 0; VECTOR wall;
        stage->ResolveSphere(position, velocity, radius, grounded, impact, wall, wallDepth);
        if (impact > 0.025f) { squashVelocity -= impact * 0.9f; wobblePower += impact * 3.0f; }

        wall.y = 0;

        if (wallDepth > 0.0001f && VSquareSize(wall) > 0.000001f)
        {
            dentDirection = VNorm(wall);
            dent = Clamp(wallDepth * 0.7f, 0.05f, 0.55f);
        }
    }
    else
    {
        for (int pass = 0; pass < 3; ++pass)
        {
            for (const BoxCollider& box : boxes) ResolveCollision(box);
        }
    }

    float fallSpeed = velocity.y < 0.0f ? -velocity.y : 0.0f;

    // 縦方向のつぶれ・伸び
    float targetScaleY = grounded ? 0.93f : 1.0f + Clamp(fallSpeed * 0.85f, 0.0f, 0.35f);

    // 目標倍率との差に比例して戻す力を加える＝ばねの動き。
    // 次の0.78倍は勢いを弱める処理で、揺れがいつまでも続くのを防ぐ。
    squashVelocity += (targetScaleY - squashScale) * 0.16f;
    squashVelocity *= 0.78f;
    squashScale = Clamp(squashScale + squashVelocity, 0.62f, 1.45f);

    // 空中での垂れ
    float targetDroop = grounded ? 0.0f : Clamp(0.08f + fallSpeed * 0.85f, 0.0f, 0.45f);

    // 目標との差の10%だけ近づける。値を一瞬で変えず、少しずつ垂れる。
    droop += (targetDroop - droop) * 0.10f;

    // 移動に応じた揺れ
    horizontalSpeed = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
    float moveRatio = Clamp(horizontalSpeed / maxSpeed, 0.0f, 1.0f);
    wobblePower += (moveRatio * 0.55f - wobblePower) * 0.06f;
    wobblePower = Clamp(wobblePower * 0.995f, 0.0f, 1.3f);

    // 揺れの角度を進める。動く速度が大きいほど揺れの周期も速くなる。
    wobblePhase += 0.12f + horizontalSpeed * 1.5f;

    // 壁のへこみを戻す
    // へこみの目標は0。ばねのように戻し、減衰させて落ち着かせる。
    dentVelocity += -dent * 0.18f;
    dentVelocity *= 0.68f;

    dent += dentVelocity;

    if (dent < 0.0f)
    {
        dent = 0.0f;
        dentVelocity = 0.0f;
    }

    // 落下したら復活
    if (position.y < -12.0f)
    {
        // 落下時は本体の位置や揺れをリセットする。選択色や設置済みの分身はここでは消えない。
        ResetBody();
    }
}

void SlimeModel::Update(const std::vector<BoxCollider>& boxes)
{
    // 数字キーで色を選択
    SelectColorByKeyboard();

    // 表情切り替え
    UpdateFace();

    BabyWorld& world = babyWorlds[this];

    bool z = CheckHitKey(KEY_INPUT_Z) != 0;
    bool x = CheckHitKey(KEY_INPUT_X) != 0;
    bool m = CheckHitKey(KEY_INPUT_M) != 0;


    // =========================================================
    // Mキー
    // 子供スライムを全員破裂
    // =========================================================
    if (m && !world.oldM)
    {
        for (WalkingBaby& baby : world.babies)
        {
            if (!baby.bursting)
            {
                baby.bursting = true;
                baby.burstFrame = 0;

                baby.speed = 0.0f;
                baby.jumpSpeed = 0.0f;
            }
        }
    }

    world.oldM = m;


    // =========================================================
    // 子供スライム生成
    // =========================================================
    if (world.spawnTimer > 0)
    {
        --world.spawnTimer;
    }


    // Z長押し
    if (z && (!oldZ || world.spawnTimer <= 0))
    {
        SpawnSplitSlime(boxes);

        world.spawnTimer = 3;
    }


    // Xを押した瞬間
    if (x && !world.oldX)
    {
        int missing =
            BATCH_BABIES -
            static_cast<int>(world.babies.size());

        for (int i = 0; i < missing; ++i)
        {
            SpawnSplitSlime(boxes);
        }
    }


    oldZ = z;
    world.oldX = x;


    // =========================================================
    // 子供スライムを動かす
    // =========================================================
    UpdateWalkingBabies(
        world,
        radius * BABY_SCALE * BABY_WIDTH,
        boxes
    );


    // =========================================================
    // 本体スライムを動かす
    // =========================================================
    UpdateBody(boxes);

    // Lキーで元の色を復元しているとき、既存のPNG破裂跡も取り除く。
    // Stage側のメッシュ塗装もStage::Updateで同時に消える。
    if (CheckHitKey(KEY_INPUT_L))
    {
        const float restoreRadius = radius * 0.72f;
        const float floorY = position.y - radius;
        for (int i = static_cast<int>(world.burstStains.size()) - 1; i >= 0; --i)
        {
            const BurstStain& stain = world.burstStains[i];
            const float dx = stain.center.x - position.x;
            const float dz = stain.center.z - position.z;
            const float reach = restoreRadius + stain.radius * 0.35f;
            if (dx * dx + dz * dz <= reach * reach && fabsf(stain.center.y - floorY) < 0.45f)
            {
                world.burstStains.erase(world.burstStains.begin() + i);
            }
        }
    }

    // =========================================================
    // ★ 本体スライムが子供を踏みつけた判定
    // =========================================================

    // 子供スライムの横方向の大きさ
    const float babyRadius =
        radius * BABY_SCALE * BABY_WIDTH;

    // 本体スライムの実際の足元の半径
    //
    // BODY_WIDTH = 1.20
    // BASE_RADIUS = 約0.42
    //
    // 見た目の底面くらいの大きさにする
    const float mainFootRadius =
        radius * BODY_WIDTH * 0.42f;


    // 本体スライムの一番下
    const float mainBottomY =
        position.y - radius;


    for (WalkingBaby& baby : world.babies)
    {
        // すでに破裂中なら何もしない
        if (baby.bursting)
        {
            continue;
        }


        // -----------------------------------------------------
        // XZ平面で、本体と子供の距離を調べる
        // -----------------------------------------------------

        float dx =
            baby.foot.x - position.x;

        float dz =
            baby.foot.z - position.z;


        float distanceXZ =
            sqrtf(dx * dx + dz * dz);


        // 本体の足元 + 子供の大きさ
        const float stompRange =
            mainFootRadius +
            babyRadius * 0.60f;


        // 横方向で踏んでいない
        if (distanceXZ > stompRange)
        {
            continue;
        }


        // -----------------------------------------------------
        // 高さを調べる
        // -----------------------------------------------------

        // 子供スライムの高さ
        float babyHeight =
            radius *
            BABY_SCALE *
            BABY_HEIGHT;


        // 子供スライムの頭
        float babyTopY =
            baby.foot.y +
            baby.jump +
            babyHeight;


        // 本体の足元より子供がかなり下なら
        // 違う階にいるので踏みつけではない
        if (babyTopY < mainBottomY - 0.15f)
        {
            continue;
        }


        // 子供の足元が本体よりかなり上なら
        // 上の階にいる可能性がある
        if (baby.foot.y > mainBottomY + 0.30f)
        {
            continue;
        }


        // =====================================================
        // ★ この子だけ破裂
        // =====================================================
        baby.bursting = true;
        baby.burstFrame = 0;

        baby.speed = 0.0f;
        baby.jumpSpeed = 0.0f;
    }
}

void SlimeModel::UpdateMesh(const std::vector<BoxCollider>& boxes)
{
    if (vertices.empty())
    {
        return;
    }

    float scaleY = squashScale;
    float scaleXZ = 1.0f / sqrtf(scaleY);

    // 同じ位相で揺らし、旋回するような揺れを避ける
    float sway = sinf(wobblePhase) * wobblePower;

    float swayX = sway * SWAY_X;
    float swayZ = sway * SWAY_Z;


    int index = 0;

    for (int y = 0; y <= LAT; ++y)
    {
        float v = static_cast<float>(y) / LAT;

        // 各高さに輪を作り、積み重ねて体にする。ringRadiusは輪の半径、heightFromBottomは底からの高さ。
        float ringRadius = 0.0f;
        float heightFromBottom = 0.0f;


        // 最も横に広がる位置
        const float BELLY_START = 0.55f;

        // 丸い下半身から平らな底面へ切り替わる位置
        const float BASE_START = 0.90f;

        // お腹の高さ（全高に対する割合）
        const float BELLY_HEIGHT = 0.32f;

        // 底面の半径（最大幅に対する割合）
        const float BASE_RADIUS = 0.42f;

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
        float lowerWeight = Clamp(1.0f - heightRate / 0.45f, 0.0f, 1.0f);
        lowerWeight *= lowerWeight;

        // 円周を流れる波ではなく、上下方向の小さな膨らみ
        float ripple = 1.0f + sinf(wobblePhase * 1.4f) * RIPPLE_POWER * wobblePower * upperWeight;

        for (int x = 0; x <= LON; ++x)
        {
            float u = static_cast<float>(x) / LON;

            // 両端の座標を完全に一致させる
            float phi = (x == LON) ? 0.0f : u * DX_PI_F * 2.0f;

            // cosとsinで輪の周りの向きを作る。その向きに輪の半径を掛けると円周上の点になる。
            float directionX = cosf(phi);
            float directionZ = sinf(phi);

            // 底面は当たり判定の下端に合わせる。
            // 縦に伸縮しても接地位置を変えない。
            // 名前にnormalがあるが、これは法線ではなく「通常スライム形状のローカル位置」。
            // Yを-radiusから作るので、底面を基準に上方向へ伸縮できる。
            VECTOR normalLocal = VGet(directionX * ringRadius * radius * scaleXZ * ripple + upperWeight * swayX * radius, -radius + heightFromBottom * radius * scaleY, directionZ * ringRadius * radius * scaleXZ * ripple + upperWeight * swayZ * radius);

            // ------------------------------------------------
            // 足場からはみ出た部分を垂らす
            // ------------------------------------------------
            if (lowerWeight > 0.0f)
            {
                VECTOR preWorld = VAdd(position, normalLocal);
                float support = GetSupportAmount(preWorld.x, preWorld.z, position.y, radius, boxes);

                // 支えが十分なら0、支えがなければ1。大きいほど足元を下げる。
                float unsupported = 1.0f - support;

                // 接地中は支えのない部分だけ垂らす。
                // 空中ではdroopで全体の垂れを表現する。
                float edgeDroop = grounded ? unsupported * 0.35f : 0.0f;
                float airDroop = grounded ? 0.0f : droop * 0.40f;

                normalLocal.y -= (edgeDroop + airDroop) * lowerWeight * radius;

                float stretch = 1.0f + unsupported * lowerWeight * 0.04f;

                normalLocal.x *= stretch;
                normalLocal.z *= stretch;
            }

            // ------------------------------------------------
            // 壁側のへこみ
            // ------------------------------------------------
            if (dent > 0.001f)
            {
                // 円周方向と壁方向の内積。正なら壁側の頂点なので、その部分をへこませる。
                float side = directionX * dentDirection.x + directionZ * dentDirection.z;

                if (side > 0.0f)
                {
                    // 頭頂部と底面中央では方向依存をなくす
                    float radialWeight = Clamp(ringRadius / BODY_WIDTH, 0.0f, 1.0f);
                    float weight = side * side * radialWeight;

                    normalLocal.x -= dentDirection.x * dent * weight * radius;
                    normalLocal.z -= dentDirection.z * dent * weight * radius;

                    // 底面を基準に上方向へ膨らませる
                    normalLocal.y += heightFromBottom * radius * dent * weight * 0.12f;
                }
            }

            // 本体は常に虹色。数字キーの選択色は赤ちゃんにだけ使う。
            VECTOR local = normalLocal;

            // 本体の位置＋本体から見た頂点位置で、ステージ内の座標に変換する。
            vertices[index].pos = VAdd(position, local);

            // 法線はこの後、実際の形から再計算する
            vertices[index].norm = VGet(0.0f, 0.0f, 0.0f);

            // difは頂点の色(R,G,B,A)。Aは不透明度で、255が不透明。spcは光沢の色。
            vertices[index].dif = RainbowColor(0.5f + directionX * ringRadius / (BODY_WIDTH * 2.0f), heightRate);

            vertices[index].spc = GetColorU8(18, 18, 18, 255);

            // u・vは画像を貼るための座標。今回の描画では画像を指定していない。
            vertices[index].u = u;
            vertices[index].v = v;

            vertices[index].su = 0.0f;
            vertices[index].sv = 0.0f;

            ++index;
        }
    }

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

// 表情描画
void SlimeModel::DrawFace()
{
    // 0 は「表情なし」
    if (currentFace == 0)
    {
        return;
    }

    // currentFaceは1～5。0は表情なし。
    int graphHandle = currentFace >= 1 && currentFace <= 5
        ? faceHandles[currentFace - 1] : -1;

    // 画像の読み込み失敗時は何もしない
    if (graphHandle < 0)
    {
        return;
    }

    if (vertices.empty() || indices.empty())
    {
        return;
    }


    VECTOR cameraPosition = GetCameraPosition();
    float cameraSideZ = cameraPosition.z < position.z ? -1.0f : 1.0f;
    const float MOVE_DIRECTION_POWER = 1.00f;
    const float CAMERA_VISIBLE_POWER = 0.55f;
    VECTOR faceForward = VGet(faceTurnX * MOVE_DIRECTION_POWER, 0.0f, cameraSideZ * CAMERA_VISIBLE_POWER);

    if (VSquareSize(faceForward) < 0.000001f)
    {
        faceForward = VGet(1.0f, 0.0f, cameraSideZ * CAMERA_VISIBLE_POWER);
    }

    faceForward = VNorm(faceForward);

    VECTOR worldUp = VGet(0.0f, 1.0f, 0.0f);
    VECTOR faceRight = CrossVector(worldUp, faceForward);

    if (VSquareSize(faceRight) < 0.000001f)
    {
        faceRight = VGet(1.0f, 0.0f, 0.0f);
    }
    else
    {
        faceRight = VNorm(faceRight);
    }

    VECTOR faceUp = CrossVector(faceForward, faceRight);

    if (VSquareSize(faceUp) < 0.000001f)
    {
        faceUp = VGet(0.0f, 1.0f, 0.0f);
    }
    else
    {
        faceUp = VNorm(faceUp);
    }

    // 顔全体の横幅と高さ
    const float faceWidth = radius * 1.55f;
    const float faceHeight = radius * 1.35f;

    // 顔の中心を少し下へ移動
    // -0.10fをさらに小さくすると、もっと下へ移動する
    VECTOR faceCenter = VAdd(position, VScale(faceUp, radius * -0.10f));

    std::vector<VERTEX3D> faceVertices;
    std::vector<unsigned short> faceIndices;

    // 最悪の場合は元メッシュと同程度なので先に容量を確保
    faceVertices.reserve(indices.size());
    faceIndices.reserve(indices.size());

    // 三角形単位で調べる。
    // 顔の範囲に入っている三角形だけを顔メッシュへコピーする。
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
            // 0より少し大きくすることで、真横まで顔が回り込むのを防ぐ。
            float facing = VDot(src.norm, faceForward);

            if (facing < 0.18f)
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
            // 少し余裕を持たせると端が欠けにくい。
            const float margin = 0.04f;

            if (u < -margin || u > 1.0f + margin || v < -margin || v > 1.0f + margin)
            {
                validTriangle = false;
                break;
            }

            temp[corner] = src;

            // 本体と完全に同じ位置だとちらつくため、
            // 法線方向へごくわずかに浮かせる。
            temp[corner].pos = VAdd(src.pos, VScale(src.norm, radius * 0.012f));

            // 顔画像のUV
            temp[corner].u = Clamp(u, 0.0f, 1.0f);
            temp[corner].v = Clamp(v, 0.0f, 1.0f);
            temp[corner].su = 0.0f;
            temp[corner].sv = 0.0f;

            // 顔画像そのものの色を使いたいので白にする。
            // PNG側の透明度がそのまま利用される。
            temp[corner].dif = GetColorU8(255, 255, 255, 255);
            temp[corner].spc = GetColorU8(0, 0, 0, 0);
        }

        if (!validTriangle)
        {
            continue;
        }

        // この三角形の3頂点を顔用配列へ追加する。
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

    // ========================================================
    // 顔描画
    // ========================================================

    // 透明PNGを通常のアルファブレンドで描く。
    DxLib::SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    DrawPolygonIndexed3D(faceVertices.data(), static_cast<int>(faceVertices.size()), faceIndices.data(), static_cast<int>(faceIndices.size() / 3), graphHandle, TRUE);
    DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}


// 描画
void SlimeModel::Draw(const std::vector<BoxCollider>& boxes)
{
    if (vertices.empty() || indices.empty())
    {
        return;
    }

    // 先に赤ちゃんを描画する
    DrawSplitSlimes();

    UpdateMesh(boxes);

    // 操作中のスライム
    // 参考画像に合わせ、色がくっきり見える不透明な体にする。
    DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 頂点配列と接続番号で三角形をまとめて描く。三角形数は接続番号の数÷3。
    // DX_NONE_GRAPHは画像なし。TRUEは透過を有効にする指定。
    DrawPolygonIndexed3D(vertices.data(), static_cast<int>(vertices.size()), indices.data(), static_cast<int>(indices.size() / 3), DX_NONE_GRAPH, TRUE);

    // 後から描くものに半透明設定が残らないよう、通常の描画設定へ戻す。
    DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // スライム本体の上に表情PNGを重ねる
    DrawFace();

    // 画面左上。床が明るくても読めるよう黒い影を付ける。
    // 実際に生成された数なので、床がなく生成できなかった分は数えない。
    int babyCount = static_cast<int>(babyWorlds[this].babies.size());
    DrawFormatString(22, 22, GetColor(0, 0, 0), "子供スライム: %d / %d", babyCount, MAX_BABIES);
    DrawFormatString(20, 20, GetColor(255, 255, 255), "子供スライム: %d / %d", babyCount, MAX_BABIES);
}


VECTOR SlimeModel::GetPosition() const
{
    return position;
}

// 1 : 赤、2 : 青、3 : 緑
int SlimeModel::GetSelectedModel() const
{
    return selectedColor;
}


void SlimeModel::SpawnSplitSlime(const std::vector<BoxCollider>& boxes)
{
    BabyWorld& world = babyWorlds[this];
    if (world.babies.size() >= MAX_BABIES || selectedColor < 1 || selectedColor > 3) return;
    const float r = radius * BABY_SCALE * BABY_WIDTH;
    // 周囲のいろいろな位置を試す。赤ちゃん同士はすり抜ける。
    // 密集時にも生成でき、全組合せの衝突計算を避けられる。
    for (int attempt = 0; attempt < 40; ++attempt)
    {
        float angle = BabyRandom(0.0f, 2.0f * DX_PI_F);
        float distance = BabyRandom(radius * 1.3f, radius * 3.2f);
        float x = position.x + cosf(angle) * distance;
        float z = position.z + sinf(angle) * distance;
        float y;
        if (!BabyFloor(x, z, position.y - radius, r, boxes, y)) continue;
        WalkingBaby baby;
        baby.foot = VGet(x, y, z);
        if (Stage::Active()) Stage::Active()->FindFloor(x, z, y - 0.01f, y + 0.01f, y, &baby.groundNormal);
        baby.lastPaint = baby.foot;
        baby.angle = angle;
        baby.facing = angle;
        baby.speed = BABY_SPEED * BabyRandom(0.65f, 1.35f);
        baby.phase = BabyRandom(0.0f, DX_PI_F * 2.0f);
        baby.turnTimer = 20 + GetRand(100);
        baby.jumpTimer = 90 + GetRand(250);
        baby.color = selectedColor; // 生まれた時の色を保持
        world.babies.push_back(baby);
        return;
    }
}

void SlimeModel::DrawSplitSlimes()
{
    BabyWorld& world = babyWorlds[this];
    DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBabyPaint(world);

    // 破裂後に地面へ残る r.png / b.png / g.png の跡。
    // 子供本体が消えた後も描画する。
    DrawBurstStains(world);

    // 本体が全部消えた後も、水滴と波紋だけは寿命まで描画する。
    if (world.babies.empty())
    {
        DrawWaterBurst(world);
        return;
    }

    // 体と目の形を初回だけ作る。以降は座標変換だけで済ませる。
    if (world.meshIndices.empty())
    {
        for (int y = 0; y < BABY_LAT; ++y)
            for (int x = 0; x < BABY_LON; ++x)
            {
                int a = y * (BABY_LON + 1) + x, b = a + BABY_LON + 1;
                unsigned short ids[6] = { static_cast<unsigned short>(a), static_cast<unsigned short>(b), static_cast<unsigned short>(a + 1), static_cast<unsigned short>(a + 1), static_cast<unsigned short>(b), static_cast<unsigned short>(b + 1) };
                world.meshIndices.insert(world.meshIndices.end(), ids, ids + 6);
            }
        for (int color = 0; color < 3; ++color)
            for (int y = 0; y <= BABY_LAT; ++y)
                for (int x = 0; x <= BABY_LON; ++x)
                {
                    float v = static_cast<float>(y) / BABY_LAT;
                    float phi = x == BABY_LON ? 0.0f : static_cast<float>(x) / BABY_LON * DX_PI_F * 2.0f;
                    VECTOR local = BabyShape(v, phi);
                    VERTEX3D vertex = {};
                    vertex.pos = VGet(local.x, local.y + 1.0f, local.z);
                    VECTOR n = v > 0.90f ? VGet(0.0f, -1.0f, 0.0f) : VGet(local.x / (BABY_WIDTH * BABY_WIDTH), (local.y + 1.0f) / (BABY_HEIGHT * BABY_HEIGHT), local.z / (BABY_WIDTH * BABY_WIDTH));
                    vertex.norm = VSquareSize(n) > 1.0e-12f ? VNorm(n) : VGet(0.0f, 1.0f, 0.0f);
                    vertex.dif = SlimeColor(color + 1, (local.y + 1.0f) / BABY_HEIGHT);
                    vertex.spc = GetColorU8(18, 18, 18, 255);
                    world.bodyTemplate[color].push_back(vertex);
                }

        // 正面をローカル+Zとして楕円の目を作る。表面の三角形探索は不要。
        const int EYE_SEGMENTS = 10;
        for (int eye = 0; eye < 2; ++eye)
        {
            int base = static_cast<int>(world.eyeTemplate.size());
            for (int i = 0; i <= EYE_SEGMENTS; ++i)
            {
                float a = (i - 1) * DX_PI_F * 2.0f / EYE_SEGMENTS;
                float x = (eye == 0 ? -1.0f : 1.0f) * BABY_WIDTH * 0.23f;
                float y = BABY_HEIGHT * 0.57f;
                if (i > 0) { x += cosf(a) * BABY_WIDTH * 0.064f; y += sinf(a) * BABY_HEIGHT * 0.15f; }
                float inside = 1.0f - x * x / (BABY_WIDTH * BABY_WIDTH) - y * y / (BABY_HEIGHT * BABY_HEIGHT);
                VERTEX3D v = {};
                v.pos = VGet(x, y, BABY_WIDTH * sqrtf(inside > 0.0f ? inside : 0.0f) + 0.008f);
                v.norm = VNorm(VGet(x / (BABY_WIDTH * BABY_WIDTH), y / (BABY_HEIGHT * BABY_HEIGHT), v.pos.z / (BABY_WIDTH * BABY_WIDTH)));
                v.dif = GetColorU8(8, 15, 32, 255);
                world.eyeTemplate.push_back(v);
            }
            for (int i = 0; i < EYE_SEGMENTS; ++i)
            {
                world.eyeIndices.push_back(static_cast<unsigned short>(base));
                world.eyeIndices.push_back(static_cast<unsigned short>(base + (i + 1) % EYE_SEGMENTS + 1));
                world.eyeIndices.push_back(static_cast<unsigned short>(base + i + 1));
            }
        }
    }

    // 200体でも体1回＋目1回で描画。配列は保持して毎フレーム確保しない。
    // 上限を増やした場合にも、60000頂点ごとに描画して16bitの桁あふれを防ぐ。
    auto flush = [](BabyBatch& batch)
        {
            if (!batch.vertices.empty())
            {
                DrawPolygonIndexed3D(batch.vertices.data(), static_cast<int>(batch.vertices.size()), batch.indices.data(), static_cast<int>(batch.indices.size() / 3), DX_NONE_GRAPH, TRUE);
            }
            batch.vertices.clear(); batch.indices.clear();
        };
    BabyBatch& bodies = world.bodyBatch;
    BabyBatch& eyes = world.eyeBatch;
    bodies.vertices.clear(); bodies.indices.clear();
    eyes.vertices.clear(); eyes.indices.clear();
    bodies.vertices.reserve(MAX_BABIES * (BABY_LAT + 1) * (BABY_LON + 1));
    bodies.indices.reserve(MAX_BABIES * world.meshIndices.size());
    eyes.vertices.reserve(MAX_BABIES * world.eyeTemplate.size());
    eyes.indices.reserve(MAX_BABIES * world.eyeIndices.size());
    const float r = radius * BABY_SCALE;
    for (const WalkingBaby& baby : world.babies)
    {
        float grow = baby.age < 15 ? 0.2f + 0.8f * baby.age / 15.0f : 1.0f;
        float sy = 1.0f + sinf(baby.phase) * 0.12f;
        float sx = 1.0f / sqrtf(sy);

        // --------------------------------------------------------
        // Mキー 本体破裂演出
        // 0～3F  : 破裂直前に水風船のように一瞬ふくらむ
        // 4～19F : 本体メッシュの三角形そのものが外へ裂けて飛ぶ
        // --------------------------------------------------------
        if (baby.bursting && baby.burstFrame < 4)
        {
            float t = baby.burstFrame / 4.0f;
            float pop = 1.0f + 0.45f * sinf(t * DX_PI_F * 0.5f);
            sx *= pop;
            sy *= pop;
        }
        float fx = cosf(baby.facing), fz = sinf(baby.facing);
        VECTOR up = baby.groundNormal;
        VECTOR forward = VGet(fx, 0, fz);
        forward = VNorm(VSub(forward, VScale(up, VDot(forward, up))));
        VECTOR right = CrossVector(up, forward);
        // 正面(+Z)を進行方向(fx,fz)へ回す。後ろ向きなら目も背面へ回る。
        auto append = [&](BabyBatch& batch, const std::vector<VERTEX3D>& shape,
            const std::vector<unsigned short>& triangles)
            {
                if (batch.vertices.size() + shape.size() > 60000) flush(batch);
                unsigned short base = static_cast<unsigned short>(batch.vertices.size());
                for (const VERTEX3D& source : shape)
                {
                    VERTEX3D v = source;
                    float x = source.pos.x * r * sx * grow;
                    float z = source.pos.z * r * sx * grow;
                    v.pos = VAdd(baby.foot, VAdd(VScale(right, x), VAdd(VScale(up, source.pos.y * r * sy * grow), VScale(forward, z))));
                    v.pos.y += baby.jump;
                    VECTOR n = VGet(source.norm.x / sx, source.norm.y / sy, source.norm.z / sx);
                    n = VNorm(n);
                    v.norm = VAdd(VScale(right, n.x), VAdd(VScale(up, n.y), VScale(forward, n.z)));
                    batch.vertices.push_back(v);
                }
                for (unsigned short index : triangles)
                    batch.indices.push_back(static_cast<unsigned short>(base + index));
            };
        // --------------------------------------------------------
        // 破裂後は「本体を縮めて消す」のではなく、
        // 本体を構成している三角形を1枚ずつ外側へ飛ばす。
        // これで水滴だけでなく、スライム本体そのものが裂けて見える。
        // --------------------------------------------------------
        if (baby.bursting && baby.burstFrame >= 4)
        {
            const std::vector<VERTEX3D>& shape = world.bodyTemplate[baby.color - 1];
            float t = (baby.burstFrame - 4) / 16.0f;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            // 最初は一気に裂け、その後は慣性で広がる。
            float spread = t * (1.35f + 0.85f * t);
            float pieceScale = 1.0f - 0.58f * t;
            VECTOR burstCenter = VAdd(baby.foot, VScale(up, r * 0.55f));

            for (size_t tri = 0; tri + 2 < world.meshIndices.size(); tri += 3)
            {
                if (bodies.vertices.size() + 3 > 60000) flush(bodies);

                unsigned short srcId[3] =
                {
                    world.meshIndices[tri],
                    world.meshIndices[tri + 1],
                    world.meshIndices[tri + 2]
                };

                VECTOR worldPos[3];
                VECTOR worldNorm[3];
                VECTOR center = VGet(0.0f, 0.0f, 0.0f);
                VECTOR avgNormal = VGet(0.0f, 0.0f, 0.0f);

                for (int k = 0; k < 3; ++k)
                {
                    const VERTEX3D& source = shape[srcId[k]];
                    float lx = source.pos.x * r * sx * grow;
                    float lz = source.pos.z * r * sx * grow;
                    worldPos[k] = VAdd(baby.foot, VAdd(VScale(right, lx),
                        VAdd(VScale(up, source.pos.y * r * sy * grow), VScale(forward, lz))));
                    worldPos[k].y += baby.jump;

                    VECTOR n = VGet(source.norm.x / sx, source.norm.y / sy, source.norm.z / sx);
                    if (VSquareSize(n) > 0.000001f) n = VNorm(n);
                    worldNorm[k] = VAdd(VScale(right, n.x), VAdd(VScale(up, n.y), VScale(forward, n.z)));
                    center = VAdd(center, worldPos[k]);
                    avgNormal = VAdd(avgNormal, worldNorm[k]);
                }
                center = VScale(center, 1.0f / 3.0f);

                VECTOR radial = VSub(center, burstCenter);
                if (VSquareSize(radial) > 0.000001f) radial = VNorm(radial);
                else radial = up;
                if (VSquareSize(avgNormal) > 0.000001f) avgNormal = VNorm(avgNormal);
                else avgNormal = radial;

                // 三角形ごとに少し方向を変え、均一な球状爆発にならないようにする。
                float seed = sinf(static_cast<float>(tri) * 12.9898f + baby.angle * 7.13f);
                float seed2 = cosf(static_cast<float>(tri) * 4.123f + baby.phase * 3.17f);
                VECTOR direction = VAdd(VScale(avgNormal, 0.72f), VScale(radial, 0.55f));
                direction = VAdd(direction, VScale(right, seed * 0.28f));
                direction = VAdd(direction, VScale(forward, seed2 * 0.28f));
                direction = VAdd(direction, VScale(up, 0.10f + fabsf(seed) * 0.12f));
                if (VSquareSize(direction) > 0.000001f) direction = VNorm(direction);
                else direction = radial;

                VECTOR offset = VScale(direction, r * spread * (0.65f + 0.30f * fabsf(seed)));
                // 後半は少し下へ落として、水の膜が重力で落ちる感じを出す。
                offset.y -= r * 0.85f * t * t;

                unsigned short base = static_cast<unsigned short>(bodies.vertices.size());
                for (int k = 0; k < 3; ++k)
                {
                    VERTEX3D v = shape[srcId[k]];
                    VECTOR fromCenter = VSub(worldPos[k], center);
                    v.pos = VAdd(VAdd(center, VScale(fromCenter, pieceScale)), offset);
                    v.norm = worldNorm[k];
                    bodies.vertices.push_back(v);
                }

                bodies.indices.push_back(base);
                bodies.indices.push_back(static_cast<unsigned short>(base + 1));
                bodies.indices.push_back(static_cast<unsigned short>(base + 2));
            }
        }
        else
        {
            append(bodies, world.bodyTemplate[baby.color - 1], world.meshIndices);
        }

        // 本体が裂け始めた瞬間に目は消す。
        if (!baby.bursting || baby.burstFrame < 4)
        {
            append(eyes, world.eyeTemplate, world.eyeIndices);
        }
    }
    flush(bodies);
    flush(eyes);

    // 子供本体が消えた後も、水滴だけはしばらく飛び続ける。
    DrawWaterBurst(world);
}
