#include "Texture.h"
#include"Dxlib.h"

Texture::Texture(std::string filename, VECTOR centerPosition, int transFlag)
   : mnHandle(-1)
   , mvPosition(centerPosition)
   , mnSizeX(0)
   , mnSizeY(0)
   , mnTransFlag(transFlag)

{
	//画像の読み込み
	mnHandle = LoadGraph(filename.c_str());

	// 画像のサイズを取得する
	// アドレスを教えたいので＆を使う
	GetGraphSize(mnHandle, &mnSizeX, &mnSizeY);

	// 半径を設定 (とりあえず共通で幅の半分を半径としておく)
	mfRadius = (float)mnSizeX / 2.0f;

}
Texture::~Texture()
{
	//読み込んだ画像の破棄
	DeleteGraph(mnHandle);
}

void Texture::Draw()
{
	// 画像の表示    ここは画像の左上を中心としているので真ん中に表示されない。
	// なので ｛- (mnSizeX / 2)｝を追加して、割る２して中心をもとめて真ん中と真ん中を合わせて描画している
	DrawGraph(static_cast<int>(mvPosition.x - (mnSizeX / 2)), static_cast<int>(mvPosition.y - (mnSizeY / 2)), mnHandle, mnTransFlag);
	
}

void Texture::Update()
{
	/*//Ｄキーが押された時
	if (CheckHitKey(KEY_INPUT_D))
	{
		mnPositionX += 3;
	}*/

}