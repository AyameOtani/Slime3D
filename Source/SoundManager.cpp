#include "SoundManager.h"
#include "DxLib.h"

SoundManager::SoundManager()
	: mnNowPlayingBgm((SOUND_BGM)-1)  // 初期は何も再生されていない状態
	, mnNowPlayingSe((SOUND_SE)-1)    // 初期は何も再生されていない状態
{

}

SoundManager::~SoundManager()
{

}

void SoundManager::Initialize()
{
	// BGM の読み込み
	//LoadBGM(SOUND_BGM::BGM_TITLE, "Resource/BGM/maou_14_shining_star.mp3");

	// SE の読み込み

}


void SoundManager::Finalize()
{
	// BGMの破棄
	for (auto it = mnBgmHandleList.begin(); it != mnBgmHandleList.end(); it++)
	{
		DeleteSoundMem(it->second);
	}

	// SEの破棄
	for (auto it = mnSeHandleList.begin(); it != mnSeHandleList.end(); it++)
	{
		DeleteSoundMem(it->second);
	}
}

void SoundManager::PlayBGM(SOUND_BGM bgm, bool isTop)
{
	// 現在再生されているBGMと同じ、かつ、最初からの再生ではないのであれば return する
	if (mnNowPlayingBgm == bgm && !isTop)
	{
		return;
	}

	for (auto it = mnBgmHandleList.begin(); it != mnBgmHandleList.end(); it++)
	{
		// 一致した種類のBGMがあれば
		if (it->first == bgm)
		{
			// BGMをループ再生
			PlaySoundMem(it->second, DX_PLAYTYPE_LOOP, isTop); // ループしつつバックグラウンド再生する
			// 現在の再生種類を更新
			mnNowPlayingBgm = bgm;
		}
	}
}
void SoundManager::PlaySE(SOUND_SE se)
{
	for (auto it = mnSeHandleList.begin(); it != mnSeHandleList.end(); it++)
	{
		// 一致した種類のSEがあれば
		if (it->first == se)
		{
			// SEをループ再生
			PlaySoundMem(it->second, DX_PLAYTYPE_BACK); // バックグラウンド再生する
			// 現在の再生種類を更新
			mnNowPlayingSe = se;
		}
	}
}

void SoundManager::LoadBGM(SOUND_BGM bgm, std::string filename)
{
	bool check = false; // 重複して読み込んでいるかどうか
	for (auto it = mnBgmHandleList.begin(); it != mnBgmHandleList.end(); it++)
	{
		if (it->first == bgm)
		{
			check = true;
			break;
		}
	}

	// 重複して読み込まれていたら何もしない
	if (check)
	{
		return;
	}

	// ファイル読み込み
	int handle = LoadSoundMem(filename.c_str());
	if (handle == -1) // -1だと失敗
	{
		return;   // 読み込み失敗したら何もしない
	}

	// 読み込んだハンドルをリストに追加
	// 読み込ん札ハンドルと、タイプ（種類）を一緒に追加

	// std::pair の一個めの変数（種類）がfirst とされている
	// pair の一つ目の要素を表していた

	// strust
	// {
	//  SOUND_BGM bgm;
	// int handle;
	// }
	// が、std::pair<SOUND_BGM, int>

	mnBgmHandleList.push_back(std::pair<SOUND_BGM, int>(bgm, handle));
}


void SoundManager::LoadSE(SOUND_SE se, std::string filename)
{
	bool check = false; // 重複して読み込んでいるかどうか
	for (auto it = mnSeHandleList.begin(); it != mnSeHandleList.end(); it++)
	{
		if (it->first == se)
		{
			check = true;
			break;
		}
	}

	// 重複して読み込まれていたら何もしない
	if (check)
	{
		return;
	}

	// ファイル読み込み
	int handle = LoadSoundMem(filename.c_str());
	if (handle == -1) // -1だと失敗
	{
		return;   // 読み込み失敗したら何もしない
	}

	mnSeHandleList.push_back(std::pair<SOUND_SE, int>(se, handle));
}

void SoundManager::StopBGM()
{
	for (auto it = mnBgmHandleList.begin(); it != mnBgmHandleList.end(); it++)
	{
		// 今現在再生しているBGMと比較させる
		if (it->first == mnNowPlayingBgm)
		{
			// BGMが再生されているかのif文
			//          もし再生されていたら
			if (CheckSoundMem(it->second))
			{
				StopSoundMem(it->second); // BGMの停止
				break;
			}

		}

	}
}