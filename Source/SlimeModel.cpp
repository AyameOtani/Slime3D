// ============================================================
// SlimeModel：動いて、ぷるぷる揺れて、別の形に変身するスライム
// ============================================================
// 1. Initialize()    … 最初の準備。頂点の入れ物や状態を初期化する。
// 2. LoadMorphModels() … MQOファイルから変身先の形を用意する。
// 3. Update()        … キー入力、分裂の進行、移動、当たり判定を更新する。
// 4. Draw()          … UpdateMesh()で見た目を作り、画面に描く。
// ※呼び出し元のコードはこのファイルに含まれていない。
//   使用時は初期化・読み込みの後に、更新と描画を繰り返す想定。
//
// 【操作】W/S：Z方向へ移動、A/D：X方向へ移動、Space：ジャンプ。
// 1～3：MQOの形へ変身、4：通常の形、Z：MQOの形の分身を設置。
//
// 【よく出る言葉】
// VECTOR      … X・Y・Zの3つの数。位置や移動方向を表す。
// 頂点        … 形を作る点。3点をつないだ三角形を集めて立体を作る。
// メッシュ    … 頂点と、それらをつなぐ三角形の集まり。
// 法線        … 面がどちらを向いているかを表す矢印。光の計算に使う。
// 補間        … 2つの位置や形の「途中」を、割合で計算すること。
// ローカル座標… スライム自身を基準にした位置。
// ワールド座標… ステージ全体の中での位置。
//
// 【ベクトル計算の読み方】
// VGet(x,y,z)：3つの数でベクトルを作る。
// VAdd(a,b)：足す。位置に移動量を足せば、移動後の位置になる。
// VSub(a,b)：引く。a-bは「bからaへ向かう矢印」になる。
// VScale(a,k)：各成分をk倍する。
// VSize(a)：矢印の長さ。VSquareSize(a)：長さの2乗。
// VNorm(a)：向きを保って長さを1にする。ゼロの矢印には使わない。
// VDot(a,b)：内積。相手の方向へどれだけ向いているかを調べる。
//
// 【時間の扱い】速度や重力は「更新1回あたり」の値。
// Update()を呼ぶ回数が変わると、実時間での移動・演出速度も変わる。
// このファイルはコメントで説明を加えたもの。
// ============================================================
#include "SlimeModel.h"

// sinf・cosf・sqrtfなどの数学関数に必要。角度の単位はラジアン。
#include <cmath>
// FLT_MAX（floatで表せる最大値）を、最小・最大を探す初期値に使う。
#include <cfloat>
// ifstreamでファイルを開き、sstreamで各行を数値や単語に分ける。
#include <fstream>
#include <sstream>
#include <string>

// ============================================================
// 通常スライムの形状設定
// ============================================================

// 名前のないnamespace内の設定・補助関数は、このソースファイル内で使う。
namespace
{
    // 横幅。大きくすると横に広がる
    const float BODY_WIDTH = 1.20f;

    // 高さ。小さくすると平たいスライムになる
    const float BODY_HEIGHT = 1.75f;

    // 分裂演出の長さ（更新回数）。60fpsなら約0.8秒。
    const float SPLIT_FRAMES = 48.0f;

    // 上半身の揺れ
    const float SWAY_X = 0.08f;
    const float SWAY_Z = 0.025f;

    // 表面の小さな膨らみ
    const float RIPPLE_POWER = 0.012f;

    // ベクトルの外積
    // 2本の矢印の両方に垂直な矢印を作る。面の向きの計算に使う。
    // 引数を入れ替えると向きが逆になる。長さは必ずしも1ではない。
    VECTOR CrossVector(
        const VECTOR& a,
        const VECTOR& b)
    {
        return VGet(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x);
    }

    // 頂点位置の補間
    // rate=0ならfrom、1ならto、0.5なら中間になる。
    // 式は from×(1-rate) + to×rate。この関数自体は割合を制限しない。
    VECTOR LerpVector(
        const VECTOR& from,
        const VECTOR& to,
        float rate)
    {
        return VAdd(
            VScale(from, 1.0f - rate),
            VScale(to, rate));
    }
}

// ============================================================
// コンストラクタ
// ============================================================
// オブジェクトが作られたときに呼ばれる。コロン以降はメンバの初期値。
// selectedModelは変身先、previousModelは変身前、morphRateは変身の進み具合。
SlimeModel::SlimeModel()
    : selectedModel(0),
    previousModel(0),
    morphRate(1.0f),
    oldSpace(false)
{
    ResetBody();

    for (int i = 0; i < 3; ++i)
    {
        morphTargets[i].loaded = false;
    }

    for (int i = 0; i < 4; ++i)
    {
        oldNumberKeys[i] = false;
    }
}

// ============================================================
// 範囲制限
// ============================================================

// 例：Clamp(1.5, 0, 1)は1、Clamp(-0.2, 0, 1)は0になる。
float SlimeModel::Clamp( float value, float minValue, float maxValue)
{
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;

    return value;
}

// ============================================================
// 初期化
// ============================================================

// ゲーム開始時の準備。分身や読み込み状態も消すので、モデル読み込みより先に呼ぶ。
void SlimeModel::Initialize()
{
    ResetBody();
    CreateMesh();

    selectedModel = 0;
    previousModel = 0;
    morphRate = 1.0f;

    oldSpace = false;

    for (int i = 0; i < 3; ++i)
    {
        morphTargets[i].vertices.clear();
        morphTargets[i].loaded = false;
    }

    for (int i = 0; i < 4; ++i)
    {
        oldNumberKeys[i] = false;
    }

    // 分裂したスライムも初期化
    splitSlimes.clear();
    oldZ = false;
}

// ============================================================
// 本体の状態を初期位置へ戻す
// ============================================================
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

// ============================================================
// メッシュ作成
//
// 頂点数と接続順は変更しない。
// LAT=22、LON=30なら713頂点。
// ============================================================
void SlimeModel::CreateMesh()
{
    vertices.clear();
    indices.clear();

    // 縦LAT分割・円周LON分割の点を用意する。両端の点も必要なので+1。
    // ここでは入れ物を作るだけで、実際の座標はUpdateMesh()で決める。
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
            indices.push_back( static_cast<unsigned short>(i0));
            indices.push_back( static_cast<unsigned short>(i2));
            indices.push_back( static_cast<unsigned short>(i1));

            indices.push_back( static_cast<unsigned short>(i1));
            indices.push_back( static_cast<unsigned short>(i2));
            indices.push_back( static_cast<unsigned short>(i3));
        }
    }
}
// ============================================================
// MQOの表面から変形先を作成する
//
// ・頂点番号の一致は不要
// ・MQOの頂点と面を読み込む
// ・中心から各方向へ線を伸ばして表面を探す
// ・交点がない方向は、近い面上の点で補う
//
// 今回添付された、Objectが1個で
// scale / rotation / translationが初期値のMQO用。
// ============================================================

// 成功ならtrue、読めない・対応外の内容ならfalseを返す。
// 元のMQOと同じ頂点数にするのではなく、スライムの各頂点に対応する表面位置を探す。
// 中心から見て隠れた凹みや穴などは、この方法では正確に再現できない場合がある。
// Objectの変換値はこの処理では適用しないため、初期値のデータを前提とする。
bool SlimeModel::LoadMqoVertices( const char* fileName, MorphTarget& target)
{
    target.loaded = false;
    target.vertices.clear();

    if (vertices.size() !=
        static_cast<size_t>((LAT + 1) * (LON + 1)))
    {
        return false;
    }

    std::ifstream file(fileName);

    if (!file.is_open())
    {
        return false;
    }

    // 三角形を作る3つの頂点番号だけを保存する、小さなデータ型。
    struct Triangle
    {
        int a;
        int b;
        int c;
    };

    std::vector<VECTOR> modelVertices;
    std::vector<Triangle> triangles;

    std::string line;
    int objectCount = 0;

    // ========================================================
    // 頂点・面の読み込み
    // ========================================================
    // ファイルを1行ずつ読む。行頭の単語でObject・vertex・faceを見分ける。
    while (std::getline(file, line))
    {
        std::stringstream stream(line);
        std::string command;

        stream >> command;

        if (command == "Object")
        {
            ++objectCount;

            // 複数Objectを誤って混ぜない
            if (objectCount > 1)
            {
                return false;
            }
        }
        else if (command == "vertex")
        {
            int count = 0;

            if (!(stream >> count) || count <= 0 || !modelVertices.empty())
            {
                return false;
            }

            for (int i = 0; i < count; ++i)
            {
                if (!std::getline(file, line))
                {
                    return false;
                }

                std::stringstream row(line);

                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;

                if (!(row >> x >> y >> z))
                {
                    return false;
                }

                // 無限大や「数値ではない値」が混ざると後の計算が壊れるため、ここで拒否する。
                if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
                {
                    return false;
                }

                modelVertices.push_back(VGet(x, y, z));
            }
        }
        else if (command == "face")
        {
            int count = 0;

            if (!(stream >> count) || count <= 0)
            {
                return false;
            }

            for (int i = 0; i < count; ++i)
            {
                if (!std::getline(file, line))
                {
                    return false;
                }

                std::stringstream row(line);
                int cornerCount = 0;

                if (!(row >> cornerCount))
                {
                    return false;
                }

                // 線や点は描画面として使わない
                if (cornerCount < 3)
                {
                    continue;
                }

                // 今回のモデルは三角形。
                // 四角形・多角形はここでは受け付けない。
                if (cornerCount != 3)
                {
                    return false;
                }

                // 面の行のV(0 1 2)のような部分から、三角形の頂点番号を取り出す。
                size_t begin = line.find("V(");

                if (begin == std::string::npos)
                {
                    return false;
                }

                size_t end = line.find(')', begin + 2);

                if (end == std::string::npos)
                {
                    return false;
                }

                std::stringstream indexStream(line.substr(begin + 2, end - begin - 2));

                Triangle triangle;

                if (!(indexStream >> triangle.a >> triangle.b >> triangle.c))
                {
                    return false;
                }

                triangles.push_back(triangle);
            }
        }
    }

    if (modelVertices.empty() || triangles.empty())
    {
        return false;
    }

    // 面の頂点番号が正しいか確認
    for (const Triangle& triangle : triangles)
    {
        int count = static_cast<int>(modelVertices.size());

        if (triangle.a < 0 || triangle.a >= count || triangle.b < 0 || triangle.b >= count || triangle.c < 0 || triangle.c >= count)
        {
            return false;
        }
    }

    // ========================================================
    // 中心と大きさをそろえる
    // ========================================================

    // 全頂点を囲む箱の最小座標・最大座標を求める。最初は十分大きい／小さい値にする。
    VECTOR minP = VGet(FLT_MAX, FLT_MAX, FLT_MAX);
    VECTOR maxP = VGet(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (const VECTOR& p : modelVertices)
    {
        if (p.x < minP.x) minP.x = p.x;
        if (p.y < minP.y) minP.y = p.y;
        if (p.z < minP.z) minP.z = p.z;

        if (p.x > maxP.x) maxP.x = p.x;
        if (p.y > maxP.y) maxP.y = p.y;
        if (p.z > maxP.z) maxP.z = p.z;
    }

    // 囲む箱の中心=(最小＋最大)÷2。頂点の平均位置とは異なる。
    VECTOR center = VScale(VAdd(minP, maxP), 0.5f);
    float maxDistance = 0.0f;

    for (const VECTOR& p : modelVertices)
    {
        float distance = VSize(VSub(p, center));

        if (distance > maxDistance)
        {
            maxDistance = distance;
        }
    }

    if (maxDistance < 0.0001f)
    {
        return false;
    }

    // 中心を原点へ移し、最も遠い頂点までの距離が1になるよう全体を同じ倍率で縮尺変更する。
    // 「&」付きなので、配列内の頂点そのものを書き換える。
    for (VECTOR& p : modelVertices)
    {
        p = VScale(VSub(p, center), 1.0f / maxDistance);
    }

    // ========================================================
    // 補助計算
    // ========================================================
    // autoは型を推論する指定。[]から始まる式は、その場で作る小さな関数（ラムダ式）。
    auto cross = [](const VECTOR& a, const VECTOR& b) -> VECTOR
    {
        return VGet(  a.y * b.z - a.z * b.y,   a.z * b.x - a.x * b.z,   a.x * b.y - a.y * b.x);
    };

    // 点pに最も近い三角形上の位置を取得
    auto closestOnTriangle = []( const VECTOR& p, const VECTOR& a, const VECTOR& b, const VECTOR& c) -> VECTOR
    {
        // 点pが三角形のどの側にあるかを内積で調べる。
        // 一番近い場所は「3つの頂点」「3つの辺」「面の内部」のいずれか。
        VECTOR ab = VSub(b, a);
        VECTOR ac = VSub(c, a);
        VECTOR ap = VSub(p, a);

        float d1 = VDot(ab, ap);
        float d2 = VDot(ac, ap);

        // 頂点aの外側の領域なら、最近点はa。
        if (d1 <= 0.0f && d2 <= 0.0f)
        {
            return a;
        }

        VECTOR bp = VSub(p, b);

        float d3 = VDot(ab, bp);
        float d4 = VDot(ac, bp);

        // 頂点bの外側の領域なら、最近点はb。
        if (d3 >= 0.0f && d4 <= d3)
        {
            return b;
        }

        float vc = d1 * d4 - d3 * d2;

        // 辺abに最も近い領域なら、辺の上を割合tだけ進んだ位置を返す。
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
        {
            float t = d1 / (d1 - d3);
            return VAdd(a, VScale(ab, t));
        }

        VECTOR cp = VSub(p, c);

        float d5 = VDot(ab, cp);
        float d6 = VDot(ac, cp);

        // 頂点cの外側の領域なら、最近点はc。
        if (d6 >= 0.0f && d5 <= d6)
        {
            return c;
        }

        float vb = d5 * d2 - d1 * d6;

        // 辺acに最も近い領域の処理。
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
        {
            float t = d2 / (d2 - d6);
            return VAdd(a, VScale(ac, t));
        }

        float va = d3 * d6 - d5 * d4;

        // 辺bcに最も近い領域の処理。
        if (va <= 0.0f && d4 - d3 >= 0.0f &&  d5 - d6 >= 0.0f)
        {
            float t = (d4 - d3) / ((d4 - d3) + (d5 - d6));

            return VAdd(b, VScale(VSub(c, b), t));
        }

        // 残るのは面の内部。aを出発点にab方向へv、ac方向へw進んだ位置を求める。
        float inverse = 1.0f / (va + vb + vc);

        float v = vb * inverse;
        float w = vc * inverse;

        return VAdd( a, VAdd(VScale(ab, v), VScale(ac, w)));
    };

    // ========================================================
    // 各方向についてMQOの表面を探す
    // ========================================================
    // スライムと同じ数だけ、変身後の頂点位置の入れ物を用意する。
    std::vector<VECTOR> mapped(vertices.size());

    for (int y = 0; y <= LAT; ++y)
    {
        // thetaは上から下へ0～π、phiは周囲を0～2π進む角度。球の各方向を作る。
        float theta =  static_cast<float>(y) / LAT * DX_PI_F;

        for (int x = 0; x <= LON; ++x)
        {
            float phi = (x == LON) ? 0.0f : static_cast<float>(x) / LON *  DX_PI_F * 2.0f;

            VECTOR direction;

            // 頭頂部と底面中央の重複頂点を完全に一致させる
            if (y == 0)
            {
                direction = VGet(0.0f, 1.0f, 0.0f);
            }
            else if (y == LAT)
            {
                direction = VGet(0.0f, -1.0f, 0.0f);
            }
            else
            {
                direction = VGet( sinf(theta) * cosf(phi), cosf(theta), sinf(theta) * sinf(phi));
                direction = VNorm(direction);
            }

            // 同じ方向で複数の面に当たったら、最も遠い交点を選んで外側の形を採用する。
            float farthestDistance = -1.0f;

            float nearestDistanceSquared = FLT_MAX;
            VECTOR nearestPoint = VGet(0.0f, 0.0f, 0.0f);

            for (const Triangle& triangle : triangles)
            {
                const VECTOR& a = modelVertices[triangle.a];
                const VECTOR& b = modelVertices[triangle.b];
                const VECTOR& c = modelVertices[triangle.c];

                VECTOR edge1 = VSub(b, a);
                VECTOR edge2 = VSub(c, a);

                VECTOR faceNormal = cross(edge1, edge2);

                // 面積がほぼゼロの三角形は無視
                if (VSquareSize(faceNormal) < 1.0e-12f)
                {
                    continue;
                }

                // 交点が見つからない場合に使う補完位置。
                // 半径1の球上の点から、最も近い面上の点を探す。
                VECTOR candidate = closestOnTriangle(direction, a, b, c);

                float distanceSquared = VSquareSize(VSub(candidate, direction));

                if (distanceSquared < nearestDistanceSquared)
                {
                    nearestDistanceSquared = distanceSquared;
                    nearestPoint = candidate;
                }

                // --------------------------------------------
                // 原点からのレイと三角形の交差判定
                // 表裏どちらの面でも判定する
                // --------------------------------------------
                // レイは「原点からdirectionへ伸びる半直線」。三角形との交点を計算する。
                VECTOR h = cross(direction, edge2);
                float determinant = VDot(edge1, h);

                // 面とレイがほぼ平行なら、安定して交点を求められないので次の面へ進む。
                if (fabsf(determinant) < 1.0e-9f)
                {
                    continue;
                }

                float inverse = 1.0f / determinant;

                // レイの始点は原点
                VECTOR s = VScale(a, -1.0f);

                // u・vは三角形内の位置を表す割合。u>=0、v>=0、u+v<=1なら面の内側。
                float u = VDot(s, h) * inverse;

                // 小数の丸め誤差で、辺の上の点を外側と誤判定しないための小さな余裕。
                const float tolerance = 0.000001f;

                if (u < -tolerance || u > 1.0f + tolerance)
                {
                    continue;
                }

                VECTOR q = cross(s, edge1);

                float v = VDot(direction, q) * inverse;

                if (v < -tolerance ||  u + v > 1.0f + tolerance)
                {
                    continue;
                }

                // directionの長さは1なので、この値は原点から交点までの距離になる。
                float distance = VDot(edge2, q) * inverse;

                if (distance > 0.000001f && distance > farthestDistance)
                {
                    farthestDistance = distance;
                }
            }

            int index = y * (LON + 1) + x;

            if (farthestDistance > 0.0f)
            {
                mapped[index] = VScale(direction, farthestDistance);
            }
            else if (nearestDistanceSquared < FLT_MAX)
            {
                mapped[index] = nearestPoint;
            }
            else
            {
                // 有効な面が存在しない
                return false;
            }
        }
    }

    // 計算が最後まで成功したので、結果を変身先データへ渡して使用可能にする。
    target.vertices.swap(mapped);
    target.loaded = true;

    return true;
}

// ============================================================
// 3種類の変形先モデルを読み込む
// ============================================================

bool SlimeModel::LoadMorphModels(const char* ibitu1File,  const char* ibitu2File, const char* slimeFile)
{
    bool ok1 = LoadMqoVertices(ibitu1File, morphTargets[0]);

    bool ok2 = LoadMqoVertices(ibitu2File, morphTargets[1]);

    bool ok3 = LoadMqoVertices(slimeFile, morphTargets[2]);

    // 3ファイルすべて成功した場合だけtrue。ただし成功した個々の形状は保存されている。
    return ok1 && ok2 && ok3;
}

// ============================================================
// モデル切り替え
//
// 1 : ibitu1.mqo
// 2 : ibitu2.mqo
// 3 : slime.mqo
// 4 : 通常スライム
//
// 変形途中の切り替えは、形状の飛びを防ぐため受け付けない。
// 変形完了後にキーを押し直す。
// ============================================================
void SlimeModel::SelectModelByKeyboard()
{
    const int keys[4] =
    {
        KEY_INPUT_1,
        KEY_INPUT_2,
        KEY_INPUT_3,
        KEY_INPUT_4
    };

    for (int i = 0; i < 4; ++i)
    {
        bool pressed = CheckHitKey(keys[i]) != 0;

        // 今は押されていて、前回は押されていない＝「押した瞬間」。長押しの連続切り替えを防ぐ。
        if (pressed && !oldNumberKeys[i] && morphRate >= 1.0f)
        {
            // 「条件 ? はいの場合 : いいえの場合」の式。キー4を通常形状の番号0に対応させる。
            int requested = (i == 3) ? 0 : i + 1;

            bool available = requested == 0 || morphTargets[requested - 1].loaded;

            if (available && requested != selectedModel)
            {
                previousModel = selectedModel;
                selectedModel = requested;
                morphRate = 0.0f;
            }
        }

        oldNumberKeys[i] = pressed;
    }

    // 約25更新で変形を完了する
    morphRate = Clamp( morphRate + 0.04f,  0.0f, 1.0f);
}

// ============================================================
// 箱の一番近い位置
// ============================================================

// 各座標を箱の範囲に収めて、箱の内側または表面でpに最も近い点を求める。
// pが箱の中なら、結果はp自身になる。
VECTOR SlimeModel::ClosestPoint( const VECTOR& p, const BoxCollider& box)
{
    return VGet( Clamp(p.x, box.minPos.x, box.maxPos.x), Clamp(p.y, box.minPos.y, box.maxPos.y), Clamp(p.z, box.minPos.z, box.maxPos.z));
}

// ============================================================
// スライムと箱の当たり判定
//
// 当たり判定は元コードと同じ球形の簡易判定。
// 描画形状そのものとの厳密な判定ではない。
// ============================================================

void SlimeModel::ResolveCollision( const BoxCollider& box)
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
        // 中心が箱の中、またはほぼ表面にある場合。6面までの距離を調べ、近い面から外へ出す。
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

        velocity = VSub( velocity, VScale(normal, velocityNormal));
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

            float amount = Clamp( penetration * 0.7f + impactSpeed * 2.2f,  0.05f, 0.55f);

            if (amount > dent)
            {
                dent = amount;
            }

            wobblePower += impactSpeed * 2.0f;
        }
    }
}

// ============================================================
// 指定したXZ位置の下に足場があるか
// ============================================================

// 見た目を垂らすか決める補助判定。指定XZ位置と近い高さに箱の上面があるか調べる。
// 本体の着地判定そのものはResolveCollision()で行う。
bool SlimeModel::HasSupport(float x,float z,float centerY,float r, const std::vector<BoxCollider>& boxes) const
{
    for (const BoxCollider& box : boxes)
    {
        if (x < box.minPos.x || x > box.maxPos.x ||  z < box.minPos.z || z > box.maxPos.z)
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

// ============================================================
// 足場の割合
// ============================================================
float SlimeModel::GetSupportAmount( float x, float z, float centerY, float r, const std::vector<BoxCollider>& boxes) const
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
        if (HasSupport( x + offsets[i][0] * distance,  z + offsets[i][1] * distance,  centerY,  r, boxes))
        {
            ++count;
        }
    }

    // 9点中9点なら1、0点なら0。これは9点で推定した割合で、厳密な接地面積ではない。
    return static_cast<float>(count) / 9.0f;
}

// ============================================================
// 移動・ジャンプ・ぷるぷる更新
// ============================================================

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
    float horizontalSpeed = sqrtf( velocity.x * velocity.x + velocity.z * velocity.z);

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

    // 現在位置＋移動量＝次の位置。速度を変えた後に位置を更新する。
    position = VAdd(position, velocity);

    // 当たり判定
    // 1つの箱から押し出すと別の箱に触れる場合があるため、3回繰り返してめり込みを減らす。
    for (int pass = 0; pass < 3; ++pass)
    {
        for (const BoxCollider& box : boxes)
        {
            ResolveCollision(box);
        }
    }

    float fallSpeed = velocity.y < 0.0f ? -velocity.y : 0.0f;

    // 縦方向のつぶれ・伸び
    float targetScaleY = grounded ? 0.93f : 1.0f + Clamp(fallSpeed * 0.85f, 0.0f, 0.35f);

    // 目標倍率との差に比例して戻す力を加える＝ばねの動き。
    // 次の0.78倍は勢いを弱める処理で、揺れがいつまでも続くのを防ぐ。
    squashVelocity += (targetScaleY - squashScale) * 0.16f;

    squashVelocity *= 0.78f;

    squashScale = Clamp( squashScale + squashVelocity,  0.62f, 1.45f);

    // 空中での垂れ
    float targetDroop = grounded ? 0.0f : Clamp(0.08f + fallSpeed * 0.85f, 0.0f, 0.45f);

    // 目標との差の10%だけ近づける。値を一瞬で変えず、少しずつ垂れる。
    droop += (targetDroop - droop) * 0.10f;

    // 移動に応じた揺れ
    horizontalSpeed = sqrtf( velocity.x * velocity.x + velocity.z * velocity.z);

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
        // 落下時は本体の位置や揺れをリセットする。選択形状や設置済みの分身はここでは消えない。
        ResetBody();
    }
}

// ============================================================
// 更新
// ============================================================
void SlimeModel::Update(const std::vector<BoxCollider>& boxes)
{
    // このUpdate()が毎回の処理の入口。まず変身先と変身の進行を更新する。
    SelectModelByKeyboard();

    bool splitting = false;
    for (const SplitSlime& split : splitSlimes)
    {
        if (split.spawnProgress < 1.0f) splitting = true;
    }

    // Zを押した瞬間に分裂。演出中の連続生成は防ぐ。
    bool z = CheckHitKey(KEY_INPUT_Z) != 0;

    if (z && !oldZ && !splitting)
    {
        SpawnSplitSlime(boxes);
    }

    oldZ = z;

    for (SplitSlime& split : splitSlimes)
    {
        split.spawnProgress = Clamp( split.spawnProgress + 1.0f / SPLIT_FRAMES, 0.0f, 1.0f);
        // 小数の足し算の誤差で1に届かない状態が残らないよう、ほぼ完了なら1にそろえる。
        if (split.spawnProgress > 0.99999f)
        {
            split.spawnProgress = 1.0f;
        }
    }

    // ステージと分身の当たり判定をまとめる
    // 元のステージの箱をコピーし、演出が終わった分身の箱を追加する。
    // 演出中の分身にはまだ乗れない。
    std::vector<BoxCollider> collisionBoxes = boxes;

    for (const SplitSlime& split : splitSlimes)
    {
        if (split.spawnProgress >= 1.0f)
        {
            collisionBoxes.push_back(split.collider);
        }
    }

    // 分身の上にも着地できる
    UpdateBody(collisionBoxes);
}

// ============================================================
// スライムメッシュ更新
//
// 球の座標から作らず、
// 頭 → 広い下半身 → 平らな底面の順に作る。
// ============================================================
void SlimeModel::UpdateMesh(const std::vector<BoxCollider>& boxes)
{
    if (vertices.empty())
    {
        return;
    }

    float scaleY = squashScale;
    // 縦につぶれたら横へ広げる。scaleXZ×scaleXZ×scaleY=1となる倍率。
    // この伸縮だけなら体積を保つ。他の揺れ・へこみまで含めた厳密な体積保存ではない。
    float scaleXZ = 1.0f / sqrtf(scaleY);

    // 同じ位相で揺らし、旋回するような揺れを避ける
    float sway = sinf(wobblePhase) * wobblePower;

    float swayX = sway * SWAY_X;
    float swayZ = sway * SWAY_Z;

    // 変形の開始と終了を滑らかにする
    // t²(3-2t)は、開始と終了をゆっくりにする補間用の式。tは0～1の進み具合。
    float blend = morphRate * morphRate * (3.0f - 2.0f * morphRate);

    int index = 0;

    for (int y = 0; y <= LAT; ++y)
    {
        float v = static_cast<float>(y) / LAT;

        // 各高さに輪を作り、積み重ねて体にする。ringRadiusは輪の半径、heightFromBottomは底からの高さ。
        float ringRadius = 0.0f;
        float heightFromBottom = 0.0f;

        // ====================================================
        // 頭は尖らせ、下半身は丸くふくらませる
        // ====================================================

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

            ringRadius =BODY_WIDTH * sinf(t * DX_PI_F * 0.5f);

            heightFromBottom = BODY_HEIGHT * (BELLY_HEIGHT +(1.0f - BELLY_HEIGHT) * (1.0f - t));
        }
        else if (v <= BASE_START)
        {
            // ------------------------------------------------
            // お腹 → 足元
            // 楕円のカーブで下側を丸く絞る
            // ------------------------------------------------
            float t = (v - BELLY_START) / (BASE_START - BELLY_START);
            float angle = t * DX_PI_F * 0.5f;
            ringRadius = BODY_WIDTH * ( BASE_RADIUS +(1.0f - BASE_RADIUS) * cosf(angle) );

            heightFromBottom = BODY_HEIGHT * BELLY_HEIGHT * (1.0f - sinf(angle));
        }
        else
        {
            // ------------------------------------------------
            // 小さな平らな底面
            // 接地部分だけ平らにして安定感を出す
            // ------------------------------------------------
            float t =(v - BASE_START) / (1.0f - BASE_START);
            ringRadius = BODY_WIDTH * BASE_RADIUS *(1.0f - t);
            heightFromBottom = 0.0f;
        }

        // 底なら0、頭なら1になる高さの割合。揺らす場所・垂らす場所の強さを分ける。
        float heightRate = heightFromBottom / BODY_HEIGHT;

        // 底面は揺らさず、上側ほど揺らす
        float upperWeight = heightRate * heightRate;

        // 下側ほど垂れやすくする
        float lowerWeight = Clamp( 1.0f - heightRate / 0.45f, 0.0f,1.0f);

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
            VECTOR normalLocal = VGet( directionX * ringRadius * radius * scaleXZ * ripple + upperWeight * swayX * radius, -radius + heightFromBottom * radius * scaleY, directionZ * ringRadius * radius * scaleXZ * ripple + upperWeight * swayZ * radius);

            // ------------------------------------------------
            // 足場からはみ出た部分を垂らす
            // ------------------------------------------------

            if (lowerWeight > 0.0f)
            {
                VECTOR preWorld = VAdd(position, normalLocal);

                float support = GetSupportAmount( preWorld.x,  preWorld.z, position.y, radius, boxes);

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

            // ------------------------------------------------
            // 通常形状とMQO形状の補間
            // ------------------------------------------------
            // 通常形状を初期値にする。変身前・変身先がMQOなら、それぞれ対応する頂点位置を使う。
            // MQO側の頂点には通常形状の揺れやへこみを直接加えていない。
            VECTOR from = normalLocal;
            VECTOR to = normalLocal;

            if (previousModel > 0 && morphTargets[previousModel - 1].loaded)
            {
                from = VScale( morphTargets[previousModel - 1].vertices[index], radius);
            }

            if (selectedModel > 0 && morphTargets[selectedModel - 1].loaded)
            {
                to = VScale( morphTargets[selectedModel - 1].vertices[index], radius);
            }

            // 同じ頂点番号の「変身前」と「変身後」の間を移動させる。これが形状の変身。
            VECTOR local = LerpVector(from, to, blend);

            // 本体の位置＋本体から見た頂点位置で、ステージ内の座標に変換する。
            vertices[index].pos = VAdd(position, local);

            // 法線はこの後、実際の形から再計算する
            vertices[index].norm = VGet(0.0f, 0.0f, 0.0f);

            // difは頂点の色(R,G,B,A)。Aは不透明度で、255が不透明。spcは光沢の色。
            vertices[index].dif = GetColorU8(90, 255, 180, 180);

            vertices[index].spc = GetColorU8(255, 255, 255, 255);

            // u・vは画像を貼るための座標。今回の描画では画像を指定していない。
            vertices[index].u = u;
            vertices[index].v = v;

            vertices[index].su = 0.0f;
            vertices[index].sv = 0.0f;

            ++index;
        }
    }

    // ========================================================
    // 変形後の三角形から法線を計算
    //
    // 球用の法線を使わないため、
    // 光沢も実際のスライム形状に合わせて変化する。
    // ========================================================
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

    // ========================================================
    // 円周の継ぎ目の法線をそろえる
    // ========================================================

    // 円周の始めと終わりは同じ場所の別頂点。向きもそろえて光の継ぎ目を目立ちにくくする。
    const float joinDistanceSquared = radius * radius * 0.00000001f;

    for (int y = 0; y <= LAT; ++y)
    {
        int first = y * (LON + 1);
        int last = first + LON;

        VECTOR difference = VSub(vertices[first].pos, vertices[last].pos);

        // MQOで別の位置に移動している場合は統合しない
        if (VSquareSize(difference) <= joinDistanceSquared)
        {
            VECTOR sum = VAdd(vertices[first].norm, vertices[last].norm);

            vertices[first].norm = sum;
            vertices[last].norm = sum;
        }
    }

    // ========================================================
    // 頭頂部と底面中央の重複頂点をそろえる
    // ========================================================
    for (int pole = 0; pole < 2; ++pole)
    {
        int row = (pole == 0) ? 0 : LAT;
        int first = row * (LON + 1);

        bool samePosition = true;

        for (int x = 1; x < LON; ++x)
        {
            VECTOR difference = VSub( vertices[first + x].pos, vertices[first].pos);

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
// ============================================================
// 描画
// ============================================================

void SlimeModel::Draw(
    const std::vector<BoxCollider>& boxes)
{
    if (vertices.empty() || indices.empty())
    {
        return;
    }

    // 先に不透明な分身を描画する
    DrawSplitSlimes();

    // 分身の上を「足場あり」として扱う。
    // これにより、上に乗った本体が下へ垂れるのを防ぐ。
    std::vector<BoxCollider> supportBoxes = boxes;

    for (const SplitSlime& split : splitSlimes)
    {
        if (split.spawnProgress >= 1.0f)
        {
            supportBoxes.push_back(split.collider);
        }
    }

    UpdateMesh(supportBoxes);

    // 操作中のスライム
    // 半透明描画を設定する。190は描画全体に使うブレンド値（最大255）。
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);

    // 頂点配列と接続番号で三角形をまとめて描く。三角形数は接続番号の数÷3。
    // DX_NONE_GRAPHは画像なし。TRUEは透過を有効にする指定。
    DrawPolygonIndexed3D( vertices.data(), static_cast<int>(vertices.size()), indices.data(), static_cast<int>(indices.size() / 3), DX_NONE_GRAPH, TRUE);

    // 後から描くものに半透明設定が残らないよう、通常の描画設定へ戻す。
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// ============================================================
// 位置取得
// ============================================================
// const付きの関数は、このオブジェクトのメンバを書き換えずに情報を返す。
VECTOR SlimeModel::GetPosition() const
{
    return position;
}

// ============================================================
// 選択中モデル取得
//
// 0 : 通常スライム
// 1 : ibitu1.mqo
// 2 : ibitu2.mqo
// 3 : slime.mqo
// ============================================================
int SlimeModel::GetSelectedModel() const
{
    return selectedModel;
}


// ============================================================
// 選択中のMQO形状で分裂する
//
// ・操作中のスライムは残す
// ・前方の床へ分身を設置する
// ・分身は動かない足場として残る
// ============================================================
void SlimeModel::SpawnSplitSlime(
    const std::vector<BoxCollider>& boxes)
{
    // 通常形状では分裂しない
    if (selectedModel < 1 || selectedModel > 3)
    {
        return;
    }

    // 変形が完了してから分裂する
    if (morphRate < 1.0f)
    {
        return;
    }

    const MorphTarget& shape = morphTargets[selectedModel - 1];

    if (!shape.loaded || shape.vertices.size() != vertices.size())
    {
        return;
    }

    // ========================================================
    // 設置方向
    //
    // 移動中は移動方向へ。
    // 停止中は右側へ設置する。
    // Aを押しながらZなら左側へ設置できる。
    // ========================================================
    float direction = 1.0f;

    if (velocity.x < -0.001f)
    {
        direction = -1.0f;
    }

    if (CheckHitKey(KEY_INPUT_A))
    {
        direction = -1.0f;
    }
    else if (CheckHitKey(KEY_INPUT_D))
    {
        direction = 1.0f;
    }

    // 本体から左右へ半径の2.8倍離した位置を候補にする。
    float spawnX = position.x + direction * radius * 2.8f;
    float spawnZ = position.z;

    // ========================================================
    // 変形先の大きさを調べる
    // ========================================================
    VECTOR localMin = VGet(FLT_MAX, FLT_MAX, FLT_MAX);
    VECTOR localMax = VGet(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (const VECTOR& vertex : shape.vertices)
    {
        VECTOR p = VScale(vertex, radius);

        if (p.x < localMin.x) localMin.x = p.x;
        if (p.y < localMin.y) localMin.y = p.y;
        if (p.z < localMin.z) localMin.z = p.z;

        if (p.x > localMax.x) localMax.x = p.x;
        if (p.y > localMax.y) localMax.y = p.y;
        if (p.z > localMax.z) localMax.z = p.z;
    }

    // ========================================================
    // 設置先の床を探す
    //
    // 分身全体が床のXZ範囲へ収まる場所だけ設置する。
    // 床のない場所には生成しない。
    // ========================================================
    // 候補位置に置ける床のうち、条件を満たす最も高い上面を探す。
    float floorY = -FLT_MAX;
    bool foundFloor = false;

    const float tolerance = 0.001f;

    for (const BoxCollider& box : boxes)
    {
        float minX = spawnX + localMin.x;
        float maxX = spawnX + localMax.x;

        float minZ = spawnZ + localMin.z;
        float maxZ = spawnZ + localMax.z;

        if (minX < box.minPos.x - tolerance || maxX > box.maxPos.x + tolerance || minZ < box.minPos.z - tolerance || maxZ > box.maxPos.z + tolerance)
        {
            continue;
        }

        // 頭より高い箱は設置先にしない
        if (box.maxPos.y > position.y + radius)
        {
            continue;
        }

        if (!foundFloor || box.maxPos.y > floorY)
        {
            floorY = box.maxPos.y;
            foundFloor = true;
        }
    }

    if (!foundFloor)
    {
        return;
    }

    // モデルの一番下が床へ接するように配置する
    // 原点Y＋モデルの底のローカルY＝床Y、になるよう原点Yを逆算する。
    VECTOR origin = VGet(spawnX, floorY - localMin.y, spawnZ);

    SplitSlime split;

    split.collider.minPos = VAdd(origin, localMin);
    split.collider.maxPos = VAdd(origin, localMax);
    split.collider.color = GetColor(70, 200, 150);

    // ========================================================
    // 箱同士が体積を持って重なっているか
    // 接しているだけなら重なりとしない
    // ========================================================
    // X・Y・Zのすべてで範囲が重なると、2つの箱は立体的に重なっている。
    auto overlaps = []( const BoxCollider& a, const BoxCollider& b) -> bool
    {
        const float epsilon = 0.001f;

        return
            a.minPos.x < b.maxPos.x - epsilon && a.maxPos.x > b.minPos.x + epsilon &&
            a.minPos.y < b.maxPos.y - epsilon && a.maxPos.y > b.minPos.y + epsilon &&
            a.minPos.z < b.maxPos.z - epsilon && a.maxPos.z > b.minPos.z + epsilon;
    };

    // 壁や別の台にめり込む場所は避ける
    for (const BoxCollider& box : boxes)
    {
        if (overlaps(split.collider, box))
        {
            return;
        }
    }

    // 既存の分身と重なる場所は避ける
    for (const SplitSlime& existing : splitSlimes)
    {
        if (overlaps(split.collider, existing.collider))
        {
            return;
        }
    }

    // ========================================================
    // 分裂した形の頂点を保存する
    // ========================================================

    // 分身用に頂点を保存する。本体が後で別の形へ変身しても、この分身の形は残る。
    split.vertices.resize(shape.vertices.size());

    for (size_t i = 0; i < shape.vertices.size(); ++i)
    {
        // {}で各要素をゼロ初期化してから、位置・色など必要な情報を設定する。
        VERTEX3D vertex = {};

        vertex.pos = VAdd( origin, VScale(shape.vertices[i], radius));
        vertex.norm = VGet(0.0f, 0.0f, 0.0f);

        // 足場は不透明にして輪郭を見やすくする
        vertex.dif = GetColorU8(65, 210, 155, 255);
        vertex.spc = GetColorU8(255, 255, 255, 255);

        int row = static_cast<int>(i) / (LON + 1);
        int column = static_cast<int>(i) % (LON + 1);

        vertex.u = static_cast<float>(column) / LON;
        vertex.v = static_cast<float>(row) / LAT;

        vertex.su = 0.0f;
        vertex.sv = 0.0f;

        split.vertices[i] = vertex;
    }

    // ========================================================
    // 分身の法線を計算する
    // ========================================================

    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        int a = indices[i];
        int b = indices[i + 1];
        int c = indices[i + 2];

        VECTOR edge1 = VSub(split.vertices[b].pos, split.vertices[a].pos);
        VECTOR edge2 = VSub(split.vertices[c].pos, split.vertices[a].pos);

        // 現在の三角形の接続順に合わせた外向きの法線
        VECTOR normal = VGet(
            edge2.y * edge1.z - edge2.z * edge1.y,
            edge2.z * edge1.x - edge2.x * edge1.z,
            edge2.x * edge1.y - edge2.y * edge1.x);

        if (VSquareSize(normal) < 1.0e-12f)
        {
            continue;
        }

        split.vertices[a].norm = VAdd(split.vertices[a].norm, normal);
        split.vertices[b].norm = VAdd(split.vertices[b].norm, normal);
        split.vertices[c].norm = VAdd(split.vertices[c].norm, normal);
    }

    // 頭頂部と底面中央の法線を統合する
    for (int pole = 0; pole < 2; ++pole)
    {
        int row = (pole == 0) ? 0 : LAT;
        int first = row * (LON + 1);

        VECTOR sum = VGet(0.0f, 0.0f, 0.0f);

        for (int x = 0; x <= LON; ++x)
        {
            sum = VAdd(sum, split.vertices[first + x].norm);
        }

        for (int x = 0; x <= LON; ++x)
        {
            split.vertices[first + x].norm = sum;
        }
    }

    // 円周の継ぎ目の法線をそろえる
    for (int y = 1; y < LAT; ++y)
    {
        int first = y * (LON + 1);
        int last = first + LON;

        VECTOR sum = VAdd( split.vertices[first].norm, split.vertices[last].norm);

        split.vertices[first].norm = sum;
        split.vertices[last].norm = sum;
    }

    for (VERTEX3D& vertex : split.vertices)
    {
        if (VSquareSize(vertex.norm) > 1.0e-12f)
        {
            vertex.norm = VNorm(vertex.norm);
        }
        else
        {
            vertex.norm = VGet(0.0f, 1.0f, 0.0f);
        }
    }

    // 完成形の頂点は配置先に保存済み。演出では生成時の本体位置からそこへ動かして見せる。
    split.spawnStart = position;
    split.spawnProgress = 0.0f;
    // 分身を一覧に追加する。Updateで演出を進め、DrawSplitSlimesで描く。
    splitSlimes.push_back(split);

    squashVelocity -= 0.10f;
    wobblePower = Clamp(wobblePower + 0.45f, 0.0f, 1.3f);
}

// ============================================================
// 分裂したスライムを描く
// ============================================================
void SlimeModel::DrawSplitSlimes()
{
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    for (const SplitSlime& split : splitSlimes)
    {
        if (split.spawnProgress < 1.0f)
        {
            float t = split.spawnProgress;
            VECTOR goal = VScale(VAdd( split.collider.minPos, split.collider.maxPos), 0.5f);

            // 0～25%: 本体付近で膨らむ
            // 25～80%: 跳ねながら離れる
            // 80～100%: 床でつぶれて元に戻る
            // 演出全体の25～80%を、移動用の0～1に変換する。前後の時間は端の値に固定する。
            float move = Clamp((t - 0.25f) / 0.55f, 0.0f, 1.0f);
            float ease = move * move * (3.0f - 2.0f * move);
            VECTOR center = LerpVector(split.spawnStart, goal, ease);
            // sinは0→1→0と変化するため、途中だけ高さを足すと跳ねる軌道になる。
            center.y += sinf(move * DX_PI_F) * radius * 0.65f;

            // 小さなサイズから完成サイズへ成長させる。sx・sy・szは各方向の伸縮倍率。
            float grow = Clamp(t / 0.65f, 0.0f, 1.0f);
            float size = 0.08f + 0.92f * grow * grow * (3.0f - 2.0f * grow);
            float stretch = sinf(move * DX_PI_F);
            float sx = size * (1.0f + stretch * 0.40f);
            float sy = size * (1.0f - stretch * 0.25f);
            float sz = size * (1.0f - stretch * 0.12f);

            if (t >= 0.80f)
            {
                float landing = Clamp((t - 0.80f) / 0.20f, 0.0f, 1.0f);
                float squash = sinf(landing * DX_PI_F) * 0.18f;
                sx = sz = 1.0f + squash;
                sy = 1.0f - squash;

                // 着地後は底面を床に固定して伸縮する。
                float halfHeight = (split.collider.maxPos.y - split.collider.minPos.y) * 0.5f;
                center = goal;
                center.y = split.collider.minPos.y + halfHeight * sy;
            }

            // 完成形を一時コピーして、そのコピーだけを演出用に変形する。完成形データは保持する。
            std::vector<VERTEX3D> animated = split.vertices;
            for (VERTEX3D& vertex : animated)
            {
                // 完成形の中心をいったん原点とし、各方向に伸縮してから演出中の中心へ移す。
                VECTOR local = VSub(vertex.pos, goal);
                local.x *= sx;
                local.y *= sy;
                local.z *= sz;
                vertex.pos = VAdd(center, local);

                // 方向ごとに違う倍率で伸縮すると面の向きも変わる。
                // 法線は倍率で割って補正し、長さ1にそろえる（拡大縮小に対する法線の変換）。
                VECTOR normal = VGet(vertex.norm.x / sx, vertex.norm.y / sy, vertex.norm.z / sz);

                if (VSquareSize(normal) > 1.0e-12f) { vertex.norm = VNorm(normal); }
            }

            DrawPolygonIndexed3D(animated.data(), static_cast<int>(animated.size()), indices.data(), static_cast<int>(indices.size() / 3), DX_NONE_GRAPH, TRUE);
            continue;
        }

        DrawPolygonIndexed3D( split.vertices.data(), static_cast<int>(split.vertices.size()), indices.data(), static_cast<int>(indices.size() / 3), DX_NONE_GRAPH, TRUE);
    }
}
