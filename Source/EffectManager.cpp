#include "EffectManager.h"
#include "EffekseerForDXLib.h"

namespace
{
	constexpr int MAX_PARTICLE = 8000;	// 最大パーティクル数
};

//*---------------------------------------------------------------------------------------
//*【?】コンストラクタ
//*----------------------------------------------------------------------------------------
EffectManager::EffectManager()
{
}


//*---------------------------------------------------------------------------------------
//*【?】デストラクタ
//*----------------------------------------------------------------------------------------
EffectManager::~EffectManager()
{

}

//*---------------------------------------------------------------------------------------
//*【?】Effekseerなどのセットアップ
//*
//* [引数] なし
//*
//* [返値]
//* true : 成功
//* false : 失敗
//*----------------------------------------------------------------------------------------
bool EffectManager::Setup()
{
	// Effekseerを初期化する。
	// 引数には画面に表示する最大パーティクル数を設定する。
	if (Effekseer_Init(MAX_PARTICLE) == -1)
	{
		MessageBox(NULL, "エフェクト初期化失敗", "EffectManager", MB_OK);
		DxLib_End();
		return -1;
	}

	
	// フルスクリーンウインドウの切り替えでリソースが消えるのを防ぐ。
	// Effekseerを使用する場合は必ず設定する。
	SetChangeScreenModeGraphicsSystemResetFlag(FALSE);

	// DXライブラリのデバイスロストした時のコールバックを設定する。
	// ウインドウとフルスクリーンの切り替えが発生する場合は必ず実行する。
	// ただし、DirectX11を使用する場合は実行する必要はない。
	Effekseer_SetGraphicsDeviceLostCallbackFunctions();
	

	// Zバッファを有効にする。
	// Effekseerを使用する場合、2DゲームでもZバッファを使用する。
	SetUseZBuffer3D(TRUE);

	// Zバッファへの書き込みを有効にする。
	// Effekseerを使用する場合、2DゲームでもZバッファを使用する。
	SetWriteZBuffer3D(TRUE);



	return true;
}


//*---------------------------------------------------------------------------------------
//*【?】Effekseerの開放
//*
//* [引数] なし
//*
//* [返値]
//* true : 成功
//* false : 失敗
//*----------------------------------------------------------------------------------------
bool EffectManager::Release()
{
	// Effekseerを終了する。
	Effkseer_End();

	return true;
}


//*---------------------------------------------------------------------------------------
//*【?】3Dエフェクトの更新
//*
//* [引数] なし
//* [返値] なし
//*----------------------------------------------------------------------------------------
void EffectManager::EffectUpdate3D()
{
	// DXライブラリのカメラとEffekseerのカメラを同期する。
	Effekseer_Sync3DSetting();

	// Effekseerにより再生中のエフェクトを更新する。
	int res = UpdateEffekseer3D();

	if (res == -1)
	{
		MessageBox(NULL, "エフェクトの更新ができませんでした", "EffectManager", MB_OK);
		assert(false);
	}
}

//*---------------------------------------------------------------------------------------
//*【?】2Dエフェクトの描画
//*
//* [引数] なし
//* [返値] なし
//*----------------------------------------------------------------------------------------
void EffectManager::EffectDraw3D()
{
	// Effekseerにより再生中のエフェクトを描画する。
	int res = DrawEffekseer3D();

	if (res == -1)
	{
		MessageBox(NULL, "エフェクトの描画ができませんでした", "EffectManager", MB_OK);
		assert(false);
	}
}


//*---------------------------------------------------------------------------------------
//*【?】2Dエフェクトの更新
//*	 ※今回は多分使わないけど一応 
//*
//* [引数] なし
//* [返値] なし
//*----------------------------------------------------------------------------------------
void EffectManager::EffectUpdate2D()
{
	// Effekseerにより再生中のエフェクトを更新する。
	UpdateEffekseer2D();
}


//*---------------------------------------------------------------------------------------
//*【?】2Dエフェクトの描画
//*	 ※今回は多分使わないけど一応 
//*
//* [引数] なし
//* [返値] なし
//*----------------------------------------------------------------------------------------
void EffectManager::EffectDraw2D()
{
	// Effekseerにより再生中のエフェクトを描画する。
	DrawEffekseer2D();
}

//*---------------------------------------------------------------------------------------
//*【?】エフェクトの読み込み
//*
//* [引数] 
//* filePath      : ファイルパス
//* magnification : 指定された値に拡大してエフェクトが読み込まれる らしいけど、基本は設定しなくてOK
//* 
//* [返値] 
//* エフェクトのハンドル 
//*----------------------------------------------------------------------------------------
int EffectManager::LoadEffect(const std::string& filePath, float magnification)
{
	 int handle = LoadEffekseerEffect(filePath.c_str(), magnification);

	 if (handle == -1)
	 {
		 MessageBox(NULL, "エフェクトが読み込めませんでした", "EffectManager", MB_OK);
		 assert(false);
	 }

	 return handle;
}

//*---------------------------------------------------------------------------------------
//*【?】3Dエフェクトの再生
//*
//* [引数] 
//* effectHandle      : エフェクトハンドル
//* 
//* [返値] 
//* エフェクトのハンドル : -1 失敗
//*----------------------------------------------------------------------------------------
int EffectManager::PlayEffect3D(int effectHandle)
{
	int handle = PlayEffekseer3DEffect(effectHandle);
	if (handle == -1)
	{
		MessageBox(NULL, "エフェクトの再生ができませんでした", "EffectManager", MB_OK);
		assert(false);
	}

	return handle;
}

//*----------------------------------------------------------------------------------------
//*【?】3Dエフェクトの位置を設定
//*----------------------------------------------------------------------------------------
bool EffectManager::SetPosEffect3D(int playingEffectHandle, float x, float y, float z)
{
	int res = SetPosPlayingEffekseer3DEffect(playingEffectHandle, x, y, z);
	return 	res == -1 ? false : true;
}
bool EffectManager::SetPosEffect3D(int playingEffectHandle, const VECTOR& pos)
{
	int res = SetPosPlayingEffekseer3DEffect(playingEffectHandle, pos.x, pos.y, pos.z);
	return 	res == -1 ? false : true;
}

//*----------------------------------------------------------------------------------------
//*【?】3Dエフェクトの角度を設定
//*----------------------------------------------------------------------------------------
bool EffectManager::SetRotationEffect3D(int playingEffectHandle, float x, float y, float z)
{
	int res = SetRotationPlayingEffekseer3DEffect(playingEffectHandle, x, y, z);
	return 	res == -1 ? false : true;
}
bool EffectManager::SetRotationEffect3D(int playingEffectHandle, const VECTOR& rot)
{
	int res = SetRotationPlayingEffekseer3DEffect(playingEffectHandle, rot.x, rot.y, rot.z);
	return 	res == -1 ? false : true;
}

//*----------------------------------------------------------------------------------------
//*【?】3Dエフェクトの拡大率を設定
//*----------------------------------------------------------------------------------------
bool EffectManager::SetScaleEffect3D(int playingEffectHandle, float x, float y, float z)
{
	int res = SetScalePlayingEffekseer3DEffect(playingEffectHandle, x, y, z);
	return 	res == -1 ? false : true;
}
bool EffectManager::SetScaleEffect3D(int playingEffectHandle, const VECTOR& rot)
{
	int res = SetScalePlayingEffekseer3DEffect(playingEffectHandle, rot.x, rot.y, rot.z);
	return 	res == -1 ? false : true;
}

//*----------------------------------------------------------------------------------------
//*【?】3Dエフェクトの再生速度を設定
//*----------------------------------------------------------------------------------------
bool EffectManager::SetSpeedEffect3D(int playingEffectHandle, float speed)
{
	int res = SetSpeedPlayingEffekseer3DEffect(playingEffectHandle, speed);
	return 	res == -1 ? false : true;
}

//*----------------------------------------------------------------------------------------
//*【?】3Dエフェクトの動的パラメータを設定
//*----------------------------------------------------------------------------------------
void EffectManager::SetDynamicParameterEffect3D(int playingEffectHandle, int32_t index, float value)
{
	SetDynamicInput3DEffect(playingEffectHandle, index, value);
}


//*----------------------------------------------------------------------------------------
//*【?】エフェクトが再生中かどうか
//*----------------------------------------------------------------------------------------
bool EffectManager::IsPlayingEffect3D(int playingEffectHandle)
{
	return IsEffekseer3DEffectPlaying(playingEffectHandle);
}