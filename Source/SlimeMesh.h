#pragma once
#include "DxLib.h"
#include <vector>

// 顔（表情画像）を貼るときの定数（本編 SlimeModel::DrawFace と同じ値）
namespace SlimeFaceConfig
{
    // 顔全体の横幅・高さ（本体の半径に対する割合）
    constexpr float kFaceWidthRate = 1.55f;
    constexpr float kFaceHeightRate = 1.35f;

    // 顔の中心を下へずらす量（本体の半径に対する割合。負の値で下へ）
    constexpr float kFaceCenterDownRate = -0.10f;

    // カメラ側を向いていない面には顔を貼らない（法線と顔の向きの内積の下限）
    constexpr float kFacingThreshold = 0.18f;

    // 顔画像の範囲外でも少し余裕を持たせる（UVの余白）
    constexpr float kUvMargin = 0.04f;

    // 本体と重なってちらつかないよう、法線方向へ浮かせる量（本体の半径に対する割合）
    constexpr float kFaceLiftRate = 0.012f;
}

// スライム本体のメッシュ（本編 SlimeModel / タイトル TitleSlimeModel 共通）。
// 本編固有の揺れ・支え・へこみは含まない。それらは呼び出し側で頂点位置を変更してから
// RecalculateNormals を呼ぶ。
class SlimeMesh
{
public:
    static const int LAT = 22;
    static const int LON = 30;

    // 横幅。大きくすると横に広がる
    static const float BODY_WIDTH;

    // 高さ。小さくすると平たいスライムになる
    static const float BODY_HEIGHT;

    // 本編の初期半径（SlimeModel::ResetBody の radius と同じ値）。タイトルの既定サイズに使う。
    static const float DEFAULT_RADIUS;

    // 段ごとの形状情報
    struct RingInfo
    {
        float ringRadius;       // 輪の半径
        float heightFromBottom; // 底からの高さ
        float heightRate;       // 底なら0、頭なら1になる高さの割合
        float upperWeight;      // 上側ほど大きい（揺らす強さ）
        float lowerWeight;      // 下側ほど大きい（垂らす強さ）
    };

    SlimeMesh();

    // インデックス（三角形のつなぎ方）と頂点配列の領域を作る
    void Initialize();

    std::vector<VERTEX3D>& GetVertices();
    const std::vector<VERTEX3D>& GetVertices() const;
    const std::vector<unsigned short>& GetIndices() const;

    // 段の形状を計算する（段番号 row は 0〜LAT）
    static void ComputeRing(int row, RingInfo& ring);

    // 列の向き（円周上の cos・sin）を計算する（列番号 col は 0〜LON）
    static void ComputeDirection(int col, float& dirX, float& dirZ);

    // 色の横方向の位置を計算する
    static float ComputeColorHorizontal(float dirX, float ringRadius);

    // 1頂点の位置・色・UVを設定する。法線は RecalculateNormals で計算する。
    void SetVertex(int row, int col, const VECTOR& pos, float heightRate, float horizontal);

    // 変形なしの基本形状を作る（タイトル用）。
    // position: 本体の位置, radius: 半径, scaleY: 縦の伸縮（squashScale）
    void BuildBaseShape(const VECTOR& position, float radius, float scaleY);

    // 形状から頂点の法線を計算する
    void RecalculateNormals(float radius);

    // 三角形をまとめて描画する。ブレンド設定は呼び出し側で行う。
    void Draw();

    // 顔（表情画像）を本体の前面に貼って描画する。
    // position: 本体の中心, radius: 半径, faceForward: 顔が向く方向（カメラ側）
    // 本体の頂点（BuildBaseShape 等で作成済み）を使う。
    void DrawFace(int graphHandle, const VECTOR& position, float radius, const VECTOR& faceForward);

private:
    std::vector<VERTEX3D> vertices;
    std::vector<unsigned short> indices;
};
