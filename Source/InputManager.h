/*
キー入力に関する補助関数
DXLibに存在しないので自作
*/

class InputManager
{
public:
	// コンストラクタ
	InputManager();
	// デストラクタ
	~InputManager();

	// 一回しか反応しないのをやってくれている処理
	// 指定されたキーが押された瞬間だけ 1 を返す関数
	static int CheckDownKey(int KeyCode);

	// 指定されたキーが離された瞬間だけ 1 を返す関数
	static int CheckUpKey(int KeyCode);

	// 指定されたキーを押し続けている間１を返す関数
	static int CheckPressKey(int KeyCode);

private:
	static int mDownBuffer[256];	// CheckDownKey用のキーバッファ
	static int mUpBuffer[256];		// CheckUpKey用のキーバッファ

};
#pragma once
