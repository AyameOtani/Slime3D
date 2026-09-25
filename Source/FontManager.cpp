#include "FontManager.h"

#include <DxLib.h>
#include <cstdarg>
#include <cstdio>

FontManager::FontManager()
{
}

FontManager::~FontManager()
{
}

void FontManager::Initialize()
{
    // 
    // 
    // にあるフォントを読み込んで使用できるようにする
    AddFontResourceEx(
        "Resource/Font/dotFont/x12y16pxMaruMonica.ttf",
        FR_PRIVATE,
        NULL
    );
}

void FontManager::Finalize()
{
    // 作成したフォントをすべて削除する
    for (auto& font : mDotFontHandles)
    {
        DeleteFontToHandle(font.second);
    }

    mDotFontHandles.clear();
}

int FontManager::GetDotFontHandle(int size)
{
    // すでに同じサイズのフォントが作られているか確認
    auto it = mDotFontHandles.find(size);

    if (it != mDotFontHandles.end())
    {
        // すでに存在するので、そのハンドルを返す
        return it->second;
    }

    // まだ存在しないので新しく作成する
    int handle = CreateDotFont(size);

    if (handle == -1)
    {
        return -1;
    }

    // サイズとフォントハンドルを保存
    mDotFontHandles[size] = handle;

    return handle;
}

int FontManager::CreateDotFont(int size)
{
    int handle = CreateFontToHandle(
        "x12y16pxMaruMonica",
        size,
        5
    );

    // フォントの作成に失敗した場合
    if (handle == -1)
    {
        printfDx("ドットフォントの作成に失敗しました。\n");

        // 標準フォントで代替
        handle = CreateFontToHandle(
            NULL,
            size,
            5
        );
    }

    return handle;
}

void FontManager::DrawDotString(
    int x,
    int y,
    int size,
    unsigned int color,
    const char* format,
    ...
    )
{
    // 指定されたサイズのフォントを取得
    int fontHandle = GetDotFontHandle(size);

    if (fontHandle == -1)
    {
        return;
    }

    // printf形式で文字列を作成する
    char buffer[1024];

    va_list args;
    va_start(args, format);

    vsnprintf(
        buffer,
        sizeof(buffer),
        format,
        args
    );

    va_end(args);

    // 作成した文字列を描画
    DrawStringToHandle(
        x,
        y,
        buffer,
        color,
        fontHandle
    );
}