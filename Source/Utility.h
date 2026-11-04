#pragma once
#include <cassert>
#include <string>
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

//////////////////////////////////////////////////////////////////////////////////////////
//
// [補助ツール群]
// イージング関数や、文字変換、デバッグ用のヘルパーをまとめています。
// 
//////////////////////////////////////////////////////////////////////////////////////////
namespace Helper
{
    //-----------------------------------------
    /*      イージング関数    */
    // 参考サイト：https://easings.net/ja
    // In : 加速
    // Out : 減速
    //-----------------------------------------
    namespace Easing
    {
        /// <summary>
        /// イージングの種類
        /// </summary>
        enum class EASE_TYPE
        {
            IN_QUAD,
            IN_CUBIC,
            IN_QUART,
            IN_QUINT,
            IN_SIN,

            OUT_QUAD,
            OUT_CUBIC,
            OUT_QUART,
            OUT_QUINT,
            OUT_SIN,

            OUT_BACK,

            OUT_BOUNCE
        };

        // --------------------------------------
        //                  加速 
        // --------------------------------------

        // 軽い加速
        inline float EaseInQuad(float _t) {
            return _t * _t;
        }

        // 強い加速
        inline float EaseInCubic(float _t) {
            return _t * _t * _t;
        }

        // かなり強い加速
        inline float EaseInQuart(float _t) {
            return _t * _t * _t * _t;
        }

        // 超強い加速
        inline float EaseInQuint(float _t) {
            return _t * _t * _t * _t * _t;
        }

        // 自然な加速
        inline float EaseInSine(float _t) {
            return 1.0f - cosf((_t * DX_PI_F) / 2.0f);
        }


        // --------------------------------------
        //                  減速
        // --------------------------------------

        // ヒュッ...シュタッって感じ
        // 2乗
        inline float EaseOutQuad(float _t) {
            float invT = 1.0f - _t;
            return 1.0f - (invT * invT);
        }

        // 3乗
        inline float EaseOutCubic(float _t) {
            float invT = 1.0f - _t;
            return 1.0f - (invT * invT * invT);
        }

        // 4乗
        inline float EaseOutQuart(float _t) {
            float invT = 1.0f - _t;
            return 1.0f - (invT * invT * invT * invT);
        }

        // 5乗
        inline float EaseOutQuint(float _t) {
            float invT = 1.0f - _t;
            return 1.0f - (invT * invT * invT * invT * invT);
        }

        inline float EaseOutSin(float _t) {
            return sinf((_t * DX_PI_F) / 2.0f);
        }


        // --------------------------------------
        //          ばねみたいなやつ
        // --------------------------------------

        // 行って戻る
        inline float EaseOutBack(float t) {
            const float c1 = 1.70158f;
            const float c3 = c1 + 1.0f;

            // (t-1) を計算
            float t1 = t - 1.0f;
            return 1.0f + c3 * FLOAT_CAST(pow(t1, 3)) + c1 * FLOAT_CAST(pow(t1, 2));
        }

        // バウンドするやつ
        inline float EaseOutBounce(float t) {
            const float n1 = 7.5625f;
            const float d1 = 2.75f;

            if (t < 1.0f / d1) {
                return n1 * t * t;
            }
            else if (t < 2.0f / d1) {
                return n1 * (t -= 1.5f / d1) * t + 0.75f;
            }
            else if (t < 2.5f / d1) {
                return n1 * (t -= 2.25f / d1) * t + 0.9375f;
            }
            else {
                return n1 * (t -= 2.625f / d1) * t + 0.984375f;
            }
        }

        /// <summary>
        /// 指定されたタイプのイージング処理を行う
        /// </summary>
        /// <param name="_type"></param>
        /// <param name="_t"></param>
        /// <returns></returns>
        inline float ApplyEasing(EASE_TYPE _type, float _t)
        {
            float rate = 0.0f;
            switch (_type)
            {
            case EASE_TYPE::IN_QUAD:
                rate = EaseInQuad(_t);
                break;
            case EASE_TYPE::IN_CUBIC:
                rate = EaseInCubic(_t);
                break;
            case EASE_TYPE::IN_QUART:
                rate = EaseInQuart(_t);
                break;
            case EASE_TYPE::IN_QUINT:
                rate = EaseInQuint(_t);
                break;
            case EASE_TYPE::IN_SIN:
                rate = EaseInSine(_t);
                break;
            case EASE_TYPE::OUT_QUAD:
                rate = EaseOutQuad(_t);
                break;
            case EASE_TYPE::OUT_CUBIC:
                rate = EaseOutCubic(_t);
                break;
            case EASE_TYPE::OUT_QUART:
                rate = EaseOutQuart(_t);
                break;
            case EASE_TYPE::OUT_QUINT:
                rate = EaseOutQuint(_t);
                break;
            case EASE_TYPE::OUT_SIN:
                rate = EaseOutSin(_t);
                break;
            case EASE_TYPE::OUT_BACK:
                rate = EaseOutBack(_t);
                break;
            case EASE_TYPE::OUT_BOUNCE:
                rate = EaseOutBounce(_t);
                break;
            default:
                break;
            }

            return rate;
        }
    };

    //-----------------------------------------
    /*       文字列の変換    */
    //-----------------------------------------
    namespace ConvertString
    {
        //*---------------------------------------------------------------------------------------
        //*【?】stringからwstringへの変換
        //*----------------------------------------------------------------------------------------
        inline std::wstring StringToWstring(const std::string& str)
        {
            std::wstring ret;
            //一度目の呼び出しは文字列数を知るため
            int len = MultiByteToWideChar(CP_UTF8,
                0,
                str.c_str(),//入力文字列
                static_cast<int>(str.length()),
                nullptr,
                0
            );
            assert(len >= 0);
            ret.resize(len);    //文字数分確保する

            //二度目の呼び出しは変換
            MultiByteToWideChar(CP_UTF8,
                0,
                str.c_str(),//入力文字列
                static_cast<int>(str.length()),
                ret.data(),
                static_cast<int>(ret.size())
            );
            return ret;
        }

        //*---------------------------------------------------------------------------------------
        //*【?】wstringからstringへの変換
        //*----------------------------------------------------------------------------------------
        inline std::string WStringToString(const std::wstring& wstr)
        {
            std::string ret;
            //一度目の呼び出しは文字列数を知るため
            int len = WideCharToMultiByte(
                CP_UTF8,
                0,
                wstr.c_str(),//入力文字列
                static_cast<int>(wstr.length()),
                nullptr,
                0,
                nullptr,
                nullptr
            );
            assert(len >= 0);
            ret.resize(len);    //文字数分確保する

            //二度目の呼び出しは変換
            WideCharToMultiByte(
                CP_UTF8,
                0,
                wstr.c_str(),//入力文字列
                static_cast<int>(wstr.length()),
                ret.data(),
                static_cast<int>(ret.size()),
                nullptr,
                nullptr
            );
            return ret;
        }
    };

    //-----------------------------------------
    /*       デバッグ関連用    */
    //-----------------------------------------
    namespace Debug
    {
        /// <summary>
        /// エラーメッセージの表示
        /// </summary>
        /// <param name="_text">本文</param>
        /// <param name="_caption">タイトル</param>
        inline void ErrorMessage(const std::wstring& _text, const std::wstring& _caption)
        {
            MessageBoxW(NULL, _text.c_str(), _caption.c_str(), MB_OK);
        }
    };
}