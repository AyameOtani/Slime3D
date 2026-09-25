#pragma once
#include "DxLib.h"

class Collision
{
public:

	Collision()
	{
	}
	~Collision()
	{
	}

	// 円と円の当たり判定関数
	static bool CheckCircleToCircle(
	  const VECTOR& centerPosA,   // Aの円の中心座標
	  const float& radiusA,        // Aの円の半径
	  const VECTOR& centerPosB,   // Bの円の中心座標
	  const float& radiusB         // Bの円の半径
	);


	// 点と円の当たり判定
	static bool CheckPointToCircle(
		const VECTOR& PointPos,
		const VECTOR& centerPos,   // 円の中心座標
		const float& radius        // 円の半径
	);

    // 矩形(四角形)と矩形の当たり判定 (AABB方式)
    // 地形(床や壁)とキャラクターの衝突判定などに使用します。
    // 左上座標(pos)とサイズ(size)から四辺を算出して重なりをチェックします。
    static bool CheckRectToRect(
        const VECTOR& posA, const VECTOR& sizeA, // 対象Aの左上座標とサイズ
        const VECTOR& posB, const VECTOR& sizeB  // 対象Bの左上座標とサイズ
    );
};
