#include "Collision.h"
#include <math.h>

// 円と円の当たり判定
bool Collision::CheckCircleToCircle(
    const VECTOR& centerPosA, const float& radiusA,
    const VECTOR& centerPosB, const float& radiusB)
{
    // 2点間の距離を求める
    float dx = centerPosA.x - centerPosB.x;
    float dy = centerPosA.y - centerPosB.y;
    float distance = (float)sqrt(dx * dx + dy * dy);

    // 半径の合計よりも距離が短ければ当たっている
    if (distance < radiusA + radiusB)
    {
        return true;
    }
    return false;
}

// 点と円の当たり判定
bool Collision::CheckPointToCircle(
    const VECTOR& PointPos,
    const VECTOR& centerPos,
    const float& radius)
{
    // 点と円の中心との距離を求める
    float dx = PointPos.x - centerPos.x;
    float dy = PointPos.y - centerPos.y;
    float distance = (float)sqrt(dx * dx + dy * dy);

    // 距離が半径より短ければ当たっている
    if (distance < radius)
    {
        return true;
    }
    return false;
}

// 矩形(四角形)と矩形の当たり判定 (AABB方式)
bool Collision::CheckRectToRect(
    const VECTOR& posA, const VECTOR& sizeA,
    const VECTOR& posB, const VECTOR& sizeB)
{
    // 対象Aの上下左右の端の座標を算出する
    float leftA = posA.x;
    float rightA = posA.x + sizeA.x;
    float topA = posA.y;
    float bottomA = posA.y + sizeA.y;

    // 対象Bの上下左右の端の座標を算出する
    float leftB = posB.x;
    float rightB = posB.x + sizeB.x;
    float topB = posB.y;
    float bottomB = posB.y + sizeB.y;

    // AとBの矩形が交差(重なっている)しているかを判定する
    // Aの右端がBの左端より右にあり、かつAの左端がBの右端より左にある（X軸の重なり）
    // かつ、Aの下端がBの上端より下にあり、かつAの上端がBの下端より上にある（Y軸の重なり）
    if (leftA < rightB && rightA > leftB &&
        topA < bottomB && bottomA > topB)
    {
        return true; // 重なっていれば当たり
    }
    return false; // 重なっていなければ外れ
}
