#pragma once

#include <map>

class FontManager
{
public:
    FontManager();
    ~FontManager();

    void Initialize();
    void Finalize();

    // ドットフォントで文字を描画する
    // size : フォントサイズ
    // color : 文字色
    // format : printfと同じように指定
    void DrawDotString(
        int x,
        int y,
        int size,
        unsigned int color,
        const char* format,
        ...
    );

private:
    // 指定されたサイズのドットフォントハンドルを取得する
    // まだ存在しなければ新しく作成する
    int GetDotFontHandle(int size);

    // ドットフォントを作成する
    int CreateDotFont(int size);

private:
    // key   : フォントサイズ
    // value : フォントハンドル
    std::map<int, int> mDotFontHandles;
};