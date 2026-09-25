#pragma once
#include "DxLib.h"
#include <vector>

// ステージの箱型当たり判定
struct BoxCollider
{
    VECTOR minPos;
    VECTOR maxPos;
    unsigned int color;
};

// スライムの移動・変形・描画をまとめたクラス
class SlimeModel
{
public:
    SlimeModel();

    void Initialize();
    bool LoadMorphModels(
        const char* ibitu1File,
        const char* ibitu2File,
        const char* slimeFile);

    void Update(const std::vector<BoxCollider>& boxes);
    void Draw(const std::vector<BoxCollider>& boxes);

    VECTOR GetPosition() const;
    int GetSelectedModel() const;

private:
    static const int LAT = 22;
    static const int LON = 30;

    struct MorphTarget
    {
        std::vector<VECTOR> vertices;
        bool loaded;
    };

    VECTOR position;
    VECTOR velocity;
    float radius;
    bool grounded;

    float squashScale;
    float squashVelocity;
    float wobblePhase;
    float wobblePower;
    float droop;
    float dent;
    float dentVelocity;
    VECTOR dentDirection;

    std::vector<VERTEX3D> vertices;
    std::vector<unsigned short> indices;
    MorphTarget morphTargets[3];

    // 0=通常、1=ibitu1、2=ibitu2、3=slime
    int selectedModel;
    int previousModel;
    float morphRate;

    bool oldSpace;
    bool oldNumberKeys[4];

    static float Clamp(float value, float minValue, float maxValue);
    static VECTOR ClosestPoint(const VECTOR& p, const BoxCollider& box);

    void ResetBody();
    void CreateMesh();
    bool LoadMqoVertices(const char* fileName, MorphTarget& target);
    void SelectModelByKeyboard();
    void ResolveCollision(const BoxCollider& box);
    bool HasSupport(float x, float z, float centerY, float r, const std::vector<BoxCollider>& boxes) const;
    float GetSupportAmount(float x, float z, float centerY, float r, const std::vector<BoxCollider>& boxes) const;
    void UpdateBody(const std::vector<BoxCollider>& boxes);
    void UpdateMesh(const std::vector<BoxCollider>& boxes);


    // ============================================================
    // 分裂して残ったスライム
    // ============================================================
    struct SplitSlime
    {
        // 完成状態の頂点
        std::vector<VERTEX3D> vertices;

        // 完成後の足場
        BoxCollider collider;

        // 分裂を開始した位置
        VECTOR spawnStart;

        // 0.0 = 分裂開始、1.0 = 完成
        float spawnProgress;
    };

    // 分裂したスライム一覧
    std::vector<SplitSlime> splitSlimes;

    // Zキーの前フレーム状態
    bool oldZ = false;

    // 分裂を作成する
    void SpawnSplitSlime(
        const std::vector<BoxCollider>& boxes);

    // 分裂したスライムを描画する
    void DrawSplitSlimes();
};
