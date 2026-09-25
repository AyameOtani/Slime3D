#include "DxLib.h"
#include "ResourceManager.h"

ResourceManager::ResourceManager()
{

}

ResourceManager::~ResourceManager()
{
    // オリジナルのハンドルを削除する

    for (int i = 0; i < resourceMapList.size(); i++)
    {
        MV1DeleteModel(resourceMapList.at(i).second);
    }

    MV1InitModel();
    InitGraph();
}

// モデルリソース生成
int ResourceManager::LoadModel(std::string pathName)
{
    // 既に読み込まれたモデルかどうか確認
    for (int i = 0; i < resourceMapList.size(); i++)
    {
        if (resourceMapList.at(i).first == pathName)
        {
            // 読み込まれているならモデルハンドルを複製して返す
            return MV1DuplicateModel(resourceMapList.at(i).second);
        }
    }

    // 読み込まれていない場合は新たに読み込む
    int handle = MV1LoadModel(pathName.c_str());
    if (handle == -1)
    {
        return -1;
    }

    // vector に追加
    resourceMapList.push_back(std::pair<std::string, int>(pathName, handle));
    return MV1DuplicateModel(handle);   // オリジナルのハンドルは残しておきたいので複製して返しておく
}

// グラフィックリソース生成
int ResourceManager::LoadGraphics(std::string pathName)
{
    for (int i = 0; i < graphicResourceMapList.size(); i++)
    {
        if (graphicResourceMapList.at(i).first == pathName)
        {
            return graphicResourceMapList.at(i).second;
        }
    }

    int handle = LoadGraph(pathName.c_str());
    if (handle == -1)
    {
        return -1;
    }

    graphicResourceMapList.push_back(std::pair<std::string, int>(pathName, handle));
    return handle;
}

// 分割されたグラフィックリソース生成
DivGraphData* ResourceManager::LoadDivGraphics(std::string pathName, int allNum, int numX, int numY)
{
    for (int i = 0; i < divGraphicResourceMapList.size(); i++)
    {
        if (divGraphicResourceMapList.at(i)->filePath == pathName)
        {
            return divGraphicResourceMapList.at(i);
        }
    }

    // 一旦テクスチャを読み込み
    int handle = LoadGraph(pathName.c_str());
    if (handle == -1)
    {
        return nullptr;
    }

    // 自作したクラスに情報を格納
    DivGraphData* data = new DivGraphData(
        pathName,
        numX, numY,
        allNum
    );

    // 一旦読み込んだテクスチャの画像サイズを取得
    int sizeX, sizeY;
    GetGraphSize(handle, &sizeX, &sizeY);

    // テクスチャ分割読み込み
    std::vector<int> test;
    handle = LoadDivGraph(pathName.c_str(), allNum, numX, numY, sizeX / numX, sizeY / numY, data->divHandleList);
    if (handle == -1)
    {
        return nullptr;
    }

    // データを保存
    divGraphicResourceMapList.push_back(data);

    return data;
}