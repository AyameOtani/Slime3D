#pragma once
// プログラム内で共通して使用する関数定義などをここに記述しておく
//　関数なども記述していく

#define INT_CAST(x)		(static_cast<int>(x))		// intにキャスト
#define UINT_CAST(x)	(static_cast<unsigned>(x))	// unsignedにキャスト
#define FLOAT_CAST(x)	(static_cast<float>(x))		// floatにキャスト
#define DOUBLE_CAST(x)	(static_cast<double>(x))	// doubleにキャスト


class Utility
{
public: // 定数定義
	static const int SCREEN_WIDTH = 1920;  // 画面の幅
	static const int SCREEN_HEIGHT = 1080; // 画面の高さ
};