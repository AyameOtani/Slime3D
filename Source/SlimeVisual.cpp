#include "SlimeVisual.h"
#include "SlimeObject.h"
#include "Stage.h"
#include "Master.h"
#include <cmath>

using namespace Game;
using namespace Helper;

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
}


//*---------------------------------------------------------------------------------------
//*【?】コンストラクタ
//*----------------------------------------------------------------------------------------
SlimeVisual::SlimeVisual()
    : 
    selectedColor(1),
    oldSpace(false),
    currentFace(0),
    faceDirectionX(1.0f),
    faceTurnX(1.0f)
{
    currentFace = 0;
    faceDirectionX = 1.0f;
    faceTurnX = 1.0f;


    for (int i = 0; i < 5; ++i) faceHandles[i] = -1;
    oldZ = false;

    for (int i = 0; i < 3; ++i)
    {
        oldNumberKeys[i] = false;
    }
}

//*---------------------------------------------------------------------------------------
//*【?】デストラクタ
//*----------------------------------------------------------------------------------------
SlimeVisual::~SlimeVisual()
{

}
//*---------------------------------------------------------------------------------------
//*【?】初期化
//*
//* [引数]なし
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeVisual::Setup()
{
    Reset();

    selectedColor = 1;

    oldSpace = false;



    // 顔画像の読み込み
    auto* resources = Master::mpGameManager->GetResourceManager();
    const char* files[5] =
    {
        "Resource\\Slime\\face_a.png",
        "Resource\\Slime\\face_b.png",
        "Resource\\Slime\\face_c.png",
        "Resource\\Slime\\face_d.png",
        "Resource\\Slime\\face_e.png"
    };

    for (int i = 0; i < 5; ++i)
    {
        if (faceHandles[i] < 0)
        {
            faceHandles[i] =
                resources->LoadGraphics(files[i]);
        }
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
}

//*---------------------------------------------------------------------------------------
//*【?】更新
//*
//* [引数]
//* slime : スライムの参照
//* 
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeVisual::Update(const SlimeObject& slime)
{
    const auto& runtime = slime.GetRuntime();

    if (runtime.splitPhase ==
        Game::Slime::SLIME_SPLIT_PHASE::PHASE_0)
    {
        UpdateRainbowDeformation(slime);
        UpdateFaceDirection(slime);
    }
    else
    {
        ++m_VisualAge;
        m_ChildPhase += 0.16f;
    }

    RefreshMesh(slime);
}

//*---------------------------------------------------------------------------------------
//*【?】描画
//*
//* [引数]
//* slime : スライムの参照
//* 
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeVisual::Draw(const SlimeObject& slime)
{
    // 操作中のスライム
    // 参考画像に合わせ、色がくっきり見える不透明な体にする。
    DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    DrawMesh(m_Body);

    if (m_IsRainbow)
    {
        DrawRainbowFace(slime);
    }
    else
    {
        DrawMesh(m_Eyes);
    }

    // 後から描くものに半透明設定が残らないよう、通常の描画設定へ戻す。
    DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

//*---------------------------------------------------------------------------------------
//*【?】シャドウ用描画
//*
//* [引数]なし
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeVisual::DrawShadow()
{
    DrawMesh(m_Body);
}

//*---------------------------------------------------------------------------------------
//*【?】メッシュの描画
//*
//* [引数]
//* mesh : メッシュ情報 
//* 
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeVisual::DrawMesh(const Mesh& mesh)
{
    if (mesh.vertices.empty() || mesh.indices.empty())
    {
        return;
    }

    // 頂点配列と接続番号で三角形をまとめて描く。三角形数は接続番号の数÷3。
    // DX_NONE_GRAPHは画像なし。TRUEは透過を有効にする指定。
    DrawPolygonIndexed3D(
        mesh.vertices.data(),
        static_cast<int>(mesh.vertices.size()),
        mesh.indices.data(),
        static_cast<int>(mesh.indices.size() / 3),
        DX_NONE_GRAPH, TRUE
    );
}


//*---------------------------------------------------------------------------------------
//*【?】時間を進めず頂点だけ更新
//*
//* [引数]
//* slime : スライムの参照
//* 
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeVisual::RefreshMesh(const SlimeObject& slime)
{
    EnsureMesh(slime);

    if (m_IsRainbow)
    {
        UpdateRainbowMesh(slime);
    }
    else
    {
        UpdateChildMesh(slime);
    }
}

//*---------------------------------------------------------------------------------------
//*【?】見た目をリセット
//*
//* [引数]なし
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeVisual::Reset()
{
    // positionは本体の基準位置、velocityは更新1回で動く量。Yのプラスは上。
    velocity = VGet(0.0f, 0.0f, 0.0f);

    // radiusは球形の当たり判定の半径。見た目の大きさにも使う。
    // groundedがtrueなら、当たり判定で地面に乗っていると判断された状態。
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


    faceDirectionX = 1.0f;
    faceTurnX = 1.0f;

    // 読み込めた最初の表情を初期表示にする。
    currentFace = 0;

    for (int i = 0; i < 5; ++i)
    {
        if (faceHandles[i] >= 0)
        {
            currentFace = i + 1;
            break;
        }
    }

    m_MeshReady = false;
}


//*---------------------------------------------------------------------------------------
//*【?】メッシュを構築する
//*     レインボースライムか子供スライムか
//*
//* [引数]
//* slime : スライムの参照
//* 
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeVisual::EnsureMesh(const SlimeObject& slime)
{
    const auto& runtime = slime.GetRuntime();

    const bool isRainbow =
        runtime.splitPhase ==
        Game::Slime::SLIME_SPLIT_PHASE::PHASE_0;

    const int color =
        runtime.color >= 1 && runtime.color <= 3
        ? runtime.color
        : 1;

    // 同じ形状・色なら、作り直さない。
    if (m_MeshReady &&
        m_IsRainbow == isRainbow &&
        (isRainbow || m_BuiltColor == color))
    {
        return;
    }

    //
    // レインボーメッシュ作成
    //
    if (isRainbow)
    {
        CreateRainbowMesh();
    }
    //
    // 子供メッシュ作成
    //
    else
    {
        CreateChildMesh(color);
    }

    m_IsRainbow = isRainbow;
    m_BuiltColor = color;
    m_MeshReady = true;
}

//*---------------------------------------------------------------------------------------
//*【?】レインボースライムメッシュの作成
//*
//* [引数]なし
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeVisual::CreateRainbowMesh()
{
    auto& vertices = m_Body.vertices;
    auto& indices = m_Body.indices;

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

//*---------------------------------------------------------------------------------------
//*【?】子供スライムメッシュの作成
//*
//* [引数]
//* color : 色
//* 
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeVisual::CreateChildMesh(int color)
{
    // 1=赤、2=青、3=緑。範囲外なら赤にする。
    if (color < 1 || color > 3)
    {
        color = 1;
    }

    // 前回の形状を取り除く。
    m_Body.vertices.clear();
    m_Body.indices.clear();
    m_Eyes.vertices.clear();
    m_Eyes.indices.clear();

    m_ChildBodySource.clear();
    m_ChildEyeSource.clear();


    // --------------------------------------------------
    // 体の頂点をつなぐインデックスを作る
    // --------------------------------------------------
    for (int y = 0; y < BABY_LAT; ++y)
    {
        for (int x = 0; x < BABY_LON; ++x)
        {
            int a = y * (BABY_LON + 1) + x, b = a + BABY_LON + 1;
            unsigned short ids[6] = 
            {
                static_cast<unsigned short>(a), 
                static_cast<unsigned short>(b), 
                static_cast<unsigned short>(a + 1),
                static_cast<unsigned short>(a + 1),
                static_cast<unsigned short>(b), 
                static_cast<unsigned short>(b + 1)
            };

            m_Body.indices.insert(m_Body.indices.end(), ids, ids + 6);
        }
    }
    // --------------------------------------------------
    // 体の変形前の頂点を作る
    // --------------------------------------------------
    for (int y = 0; y <= BABY_LAT; ++y)
    {
        for (int x = 0; x <= BABY_LON; ++x)
        {
            float v = static_cast<float>(y) / BABY_LAT;
            float phi = x == BABY_LON ? 0.0f : static_cast<float>(x) / BABY_LON * DX_PI_F * 2.0f;
            VECTOR local = BabyShape(v, phi);
            VERTEX3D vertex = {};
            vertex.pos = VGet(local.x, local.y + 1.0f, local.z);
            VECTOR n = v > 0.90f ? VGet(0.0f, -1.0f, 0.0f) : VGet(local.x / (BABY_WIDTH * BABY_WIDTH), (local.y + 1.0f) / (BABY_HEIGHT * BABY_HEIGHT), local.z / (BABY_WIDTH * BABY_WIDTH));
            vertex.norm = VSquareSize(n) > 1.0e-12f ? VNorm(n) : VGet(0.0f, 1.0f, 0.0f);
            vertex.dif = SlimeColor(color, (local.y + 1.0f) / BABY_HEIGHT);
            vertex.spc = GetColorU8(18, 18, 18, 255);
            m_ChildBodySource.push_back(vertex);
        }
    }

    // --------------------------------------------------
    // 正面をローカル+Zとして楕円の目を作る。表面の三角形探索は不要。
    // --------------------------------------------------
    const int EYE_SEGMENTS = 10;
    for (int eye = 0; eye < 2; ++eye)
    {
        int base = static_cast<int>(m_ChildEyeSource.size());
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
            m_ChildEyeSource.push_back(v);
        }
        for (int i = 0; i < EYE_SEGMENTS; ++i)
        {
            m_Eyes.indices.push_back(static_cast<unsigned short>(base));
            m_Eyes.indices.push_back(static_cast<unsigned short>(base + (i + 1) % EYE_SEGMENTS + 1));
            m_Eyes.indices.push_back(static_cast<unsigned short>(base + i + 1));
        }
    }


    // 描画・変形用の配列にも初期形状を入れる。
    m_Body.vertices = m_ChildBodySource;
    m_Eyes.vertices = m_ChildEyeSource;
}

//*---------------------------------------------------------------------------------------
//*【?】レインボースライムの変形更新
//*
//* [引数]
//* slime : スライムの参照
//* 
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeVisual::UpdateRainbowDeformation(const SlimeObject& slime)
{

}

//*---------------------------------------------------------------------------------------
//*【?】レインボースライムメッシュ情報の更新
//*
//* [引数]
//* slime : スライムの参照
//* 
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeVisual::UpdateRainbowMesh(const SlimeObject& slime)
{
    auto& vertices = m_Body.vertices;
    auto& indices = m_Body.indices;
    const VECTOR& position = slime.GetPosition();
    const float radius = slime.GetRuntime().radius;

    if ( vertices.empty() || indices.empty())
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
        float lowerWeight = Math::Clamp(1.0f - heightRate / 0.45f, 0.0f, 1.0f);
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
                float support = GetSupportAmount(preWorld.x, preWorld.z, position.y, radius);

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
                    float radialWeight = Math::Clamp(ringRadius / BODY_WIDTH, 0.0f, 1.0f);
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


//*---------------------------------------------------------------------------------------
//*【?】子供スライムメッシュ情報の更新
//*
//* [引数]
//* slime : スライムの参照
//* 
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeVisual::UpdateChildMesh(const SlimeObject& slime)
{
    const auto& runtime = slime.GetRuntime();
    const float r = runtime.radius / BABY_WIDTH;

    // 地面の法線を上方向として設定
    const VECTOR up = VNorm(runtime.groundNormal);
    VECTOR direction = slime.GetDirection();

    // 地面に対して垂直な成分を取り出す
    VECTOR normalComponent = VScale(up, VDot(direction, up));

    // 垂直な成分を引くと、地面に沿う方向が残る。
    VECTOR forward = VSub(direction, normalComponent);

    // 向きはそのままで、長さを1にそろえる。
    if (VSquareSize(forward) > 0.000001f)
    {
        forward = VNorm(forward);
    }

    // 右
    const VECTOR right = CrossVector(up, forward);


    const VECTOR foot =
        VSub(slime.GetPosition(),
            VScale(up, runtime.radius));

    const float grow = m_VisualAge < 15
        ? 0.2f + 0.8f * m_VisualAge / 15.0f
        : 1.0f;

    const float sy =
        1.0f + sinf(m_ChildPhase) * 0.12f;
    const float sx = 1.0f / sqrtf(sy);

    auto transform = [&](
        const std::vector<VERTEX3D>& source,
        std::vector<VERTEX3D>& output)
        {
            output.resize(source.size());

            for (size_t i = 0; i < source.size(); ++i)
            {
                const VERTEX3D& src = source[i];
                VERTEX3D dst = src;

                const float x = src.pos.x * r * sx * grow;
                const float y = src.pos.y * r * sy * grow;
                const float z = src.pos.z * r * sx * grow;

                dst.pos = VAdd(
                    foot,
                    VAdd(VScale(right, x),
                        VAdd(VScale(up, y),
                            VScale(forward, z))));

                VECTOR n = VNorm(VGet(
                    src.norm.x / sx,
                    src.norm.y / sy,
                    src.norm.z / sx));

                dst.norm = VAdd(
                    VScale(right, n.x),
                    VAdd(VScale(up, n.y),
                        VScale(forward, n.z)));

                output[i] = dst;
            }
        };

    transform(m_ChildBodySource, m_Body.vertices);
    transform(m_ChildEyeSource, m_Eyes.vertices);

}


//*---------------------------------------------------------------------------------------
//*【?】顔の向き更新
//*
//* [引数]
//* slime : スライムの参照
//* 
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeVisual::UpdateFaceDirection(const SlimeObject& slime)
{
    const VECTOR move = slime.GetRuntime().velocity;

    constexpr float TURN_THRESHOLD = 0.001f;

    if (move.x < -TURN_THRESHOLD)
    {
        faceDirectionX = -1.0f;
    }
    else if (move.x > TURN_THRESHOLD)
    {
        faceDirectionX = 1.0f;
    }

    // 停止中やZ方向だけの移動では、直前の左右向きを維持。
    faceTurnX += (faceDirectionX - faceTurnX) * 0.18f;
}

//*---------------------------------------------------------------------------------------
//*【?】レインボースライムの顔表示
//*
//* [引数]
//* slime : スライムの参照
//* 
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeVisual::DrawRainbowFace(const SlimeObject& slime) const
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

    const float radius = slime.GetRuntime().radius;
    auto& vertices = m_Body.vertices;
    auto& indices = m_Body.indices;

    if (vertices.empty() || indices.empty())
    {
        return;
    }

    const VECTOR& position = slime.GetPosition();

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
            temp[corner].u = Math::Clamp(u, 0.0f, 1.0f);
            temp[corner].v = Math::Clamp(v, 0.0f, 1.0f);
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

//*---------------------------------------------------------------------------------------
//*【?】表情の設定
//*
//* [引数]
//* face : 表情番号
//* 
//* [返値]なし
//*----------------------------------------------------------------------------------------
void SlimeVisual::SetFace(int face)
{
    if (face < 0 || face > 5)
    {
        return;
    }

    // 0は顔を非表示にする指定。
    if (face == 0)
    {
        currentFace = 0;
        return;
    }

    // 読み込みに失敗した画像なら、今の表情を維持。
    if (faceHandles[face - 1] < 0)
    {
        return;
    }

    currentFace = face;
}





float SlimeVisual::GetSupportAmount(float x, float z, float centerY, float r) const
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
        if (HasSupport(x + offsets[i][0] * distance, z + offsets[i][1] * distance, centerY, r))
        {
            ++count;
        }
    }

    // 9点中9点なら1、0点なら0。これは9点で推定した割合で、厳密な接地面積ではない。
    return static_cast<float>(count) / 9.0f;
}

bool SlimeVisual::HasSupport(float x, float z, float centerY, float r) const
{
    if (Stage* stage = Stage::Active())
    {
        float floorY;
        return stage->FindFloor(x, z, centerY - r * 1.65f, centerY + r * 0.25f, floorY);
    }

    return false;
}