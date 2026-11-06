#pragma once
#pragma once
#include "DxLib.h"


//マウスのクリック判定を調べるために追加島した。大谷
class Mouse
{
public:
    // マウスのx,yの位置 
    static float x, y;

    // 前フレームに押されていたかの変数
    static int input;       // 今の状態
    static int lastInput;   // 前フレームの状態

    static void Update()
    {
        int ix, iy;
        GetMousePoint(&ix, &iy); // マウスカーソルの位置を取得する
        x = (float)ix;  // floatに直してる
        y = (float)iy;
        lastInput = input;
        input = GetMouseInput(); // マウス押されているか  
    }

    //アクセサ関係
    // 押しっぱなし判定 左クリック
    static bool IsPress() { return (input & MOUSE_INPUT_LEFT); }
    // 押した瞬間判定
    static bool IsTrigger() { return (input & MOUSE_INPUT_LEFT) && !(lastInput & MOUSE_INPUT_LEFT); }
};