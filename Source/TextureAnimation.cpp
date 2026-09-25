#include "TextureAnimation.h"

TextureAnimation::TextureAnimation(
	VECTOR position,
	std::string filename,
	int allNum,
	int xNum,
	int yNum,
	int interval
)
	: mvPosition(position)
	, mnCounter(0)
	, mnInterval(interval)
	, mnCurrentNum(0)
	,mnAllNum(allNum)
{
	mnHandleList = new int[allNum];

	// 画像ファイル読み込み
	int handle = LoadGraph(filename.c_str());
	if (handle == -1)
	{
		return; // 読み込み失敗したら以降は処理しない
	}

	// サイズ取得
	int sizeX, sizeY;
	GetGraphSize(handle, &sizeX, &sizeY);

	// ここで mnxNumとmnyNum にサイズを渡している
	// ここで渡さないとサイズ取得ができない
	mnxNum = sizeX / xNum;
	mnyNum = sizeY / yNum;

	int success = LoadDivGraph(
		filename.c_str(),
		allNum,
		xNum,
		yNum,

		// サイズの分割数のやつをとりたいからここ調整必要
		sizeX / xNum,
		sizeY / yNum,

		mnHandleList
	);

	// ここで半径を設定（画像の半分に設定している）
	mfRadius = (float)mnxNum / 2.0f;
	
}

TextureAnimation::~TextureAnimation()
{

}


void TextureAnimation::Update()// 更新
{
	// カウンタをインクリメント
	mnCounter++;
	if (mnCounter % mnInterval == 0)
	{
		mnCounter = 0;  // カウンタを戻す
		mnCurrentNum++; // テクスチャ番号を進める
		if (mnCurrentNum >= mnAllNum)
		{
			mnCurrentNum = 0;  // ループさせる
		}

	}
}

void TextureAnimation::Draw()// 描画
{
	
	// ここをcenterPositionにすれば良い
	// このままだと、左上中心でそこから半径分となってしまっている
	// これをcenterPositionにすると、真ん中の座標から半径分になるので判定が正しくなる
	DrawGraph(static_cast<int>(mvPosition.x - (mnxNum / 2)), static_cast<int>(mvPosition.y - (mnyNum / 2)), mnHandleList[mnCurrentNum], true);
}