#pragma once
#include <vector>
#include <string>

// ★リソース管理クラスの使い方★
//
// Master クラスに、SceneManagerと同じような形でクラスのポインタを作る
// Main.cpp にて初期化、delete 処理を書く
// 
// Modelクラスなどで使用している 『MV1LoadModel(filename.c_str())』 を 『Master::mpResourceManager->LoadModel(filename.c_str())』
// （書き方は一例。変数名などは皆さんが書くため多少変わる可能性はあり）
// などと置き換える。
//

// ★なぜ使うのか★
// 
// 敵などを生成した際にカクつく場合、大体の原因がモデルの読み込みにあるため、
// その読み込み処理を少なくするためにリソース管理クラスを使う。
// 大量にモデルを読み込む人ほど恩恵がでかい。
// それと同じ理由で、画像ファイルなども大きなサイズを読み込むとカクつくため、
// 同じような効果が得られる。
// （画像読み込みは LoadGraphics で出来る）
//

// ★注意★
// LoadGraphics, LoadDivGraphics を使う場合、
// 現状ではそれぞれのクラスで 『DeleteGraph』 などを呼び出して、読み込んだ画像を削除している処理があるはずですが、
// その処理は消してしまって大丈夫です。
// （画像に関しては『複製』をしておらず、DeleteGraph をしてしまうと大本のデータが削除されてしてしまうため、データを使いまわすことが出来なくなる）
// 

// テクスチャの分割読み込み用データ
struct DivGraphData
{
	std::string filePath;	// テクスチャのパス名
	int* divHandleList;		// 分割されたテクスチャハンドルのリスト
	int divX;				// 横の分割数
	int divY;				// 縦の分割数
	int allNum;				// 分割最大数

	DivGraphData(
		std::string filePath,
		int divX,
		int divY,
		int allNum
	)
	{
		this->filePath = filePath;
		this->divHandleList = new int[allNum];
		this->divX = divX;
		this->divY = divY;
		this->allNum = allNum;
	}
};

// リソース管理クラス
class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();

	int LoadModel(std::string pathName);	// モデルリソース生成
	int LoadGraphics(std::string pathName);	// グラフィックリソース生成
	DivGraphData* LoadDivGraphics(std::string pathName, int allNum, int numX, int numY);	// 分割されたグラフィックリソース生成

	// 各種リソース管理マップの要素数を合計して総リソース数を算出する
	size_t GetTotalResource()
	{
		return resourceMapList.size() + graphicResourceMapList.size() + divGraphicResourceMapList.size();
	}

private:
	std::vector<std::pair<std::string, int>> resourceMapList;			// モデルリソースリスト
	std::vector<std::pair<std::string, int>> graphicResourceMapList;	// テクスチャリスト
	std::vector<DivGraphData*> divGraphicResourceMapList;				// 分割テクスチャリスト
};