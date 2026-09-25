#pragma once
#include <string>
#include <vector>

class SoundManager
{
public:   // enum, struct などの定義

	enum SOUND_BGM  // BGMの種類
	{
		BGM_TITLE = 0,  // タイトル用のBGM
		// 増やしても良い
	};

	enum SOUND_SE  // SEの種類
	{
		SE_DECIDE = 0,  // 決定音
		// 増やしても良い
	};

public:  // 公開

	SoundManager();
	~SoundManager();

	// 初期化
	void Initialize();

	// 終了処理
	void Finalize();

public:   // サウンド再生系  こっちは関数

	// BGM再生
	// SOUND_BGM... 鳴らしたいBGMの種類
	// isTop... 最初から再生するかどうか
	// bool isTop = true -> デフォルト引数と呼ばれる書き方。館数を呼びだす際に渡さなくても認識される。
	void PlayBGM(SOUND_BGM bgm, bool isTop = true);

	// SE再生
	// SEは短い音なので最初から再生のは入れなくて良い
	void PlaySE(SOUND_SE se);


	// BGMの読み込み
	void LoadBGM(SOUND_BGM bgm, std::string filename);

	// SEの読み込み
	void LoadSE(SOUND_SE se, std::string filename);


	// BGMの停止
	void StopBGM();


private:  // メンバ変数

	SOUND_BGM mnNowPlayingBgm;  // 現在再生されているBGMの種類
	SOUND_SE mnNowPlayingSe;  // 現在再生されているSEの種類


	// pair... 構造体みたいな要素？


	// よみこんだBGMハンドルのリスト
	std::vector<std::pair<SOUND_BGM, int>> mnBgmHandleList;
	// よみこんだSEハンドルのリスト
	std::vector<std::pair<SOUND_SE, int>> mnSeHandleList;

};
