#pragma once
#include "DxLib.h"
#include <vector>

struct BoxCollider
{
    VECTOR minPos;
    VECTOR maxPos;
    unsigned int color;
};

class SlimeModel
{
public:
    SlimeModel();
    void Initialize();
    void Update(const std::vector<BoxCollider>& boxes);
    void Draw(const std::vector<BoxCollider>& boxes);
    VECTOR GetPosition() const;
    float GetRadius() const { return radius; }
    int GetSelectedModel() const;
    int GetSelectedColor() const { return selectedColor; }

private:
    static const int LAT = 22;
    static const int LON = 30;
    struct SplitSlime
    {
        std::vector<VERTEX3D> vertices;
        BoxCollider collider;
        VECTOR spawnStart;
        float spawnProgress;
    };
    static float Clamp(float value, float minValue, float maxValue);
    void ResetBody();
    void CreateMesh();
    void SelectColorByKeyboard();
    void UpdateFace();
    VECTOR ClosestPoint(const VECTOR& p, const BoxCollider& box);
    void ResolveCollision(const BoxCollider& box);
    bool HasSupport(float x, float z, float centerY, float r, const std::vector<BoxCollider>& boxes) const;
    float GetSupportAmount(float x, float z, float centerY, float r, const std::vector<BoxCollider>& boxes) const;
    void UpdateBody(const std::vector<BoxCollider>& boxes);
    void UpdateMesh(const std::vector<BoxCollider>& boxes);
    void DrawFace();
    void SpawnSplitSlime(const std::vector<BoxCollider>& boxes);
    void DrawSplitSlimes();

    std::vector<VERTEX3D> vertices;
    std::vector<unsigned short> indices;
    std::vector<SplitSlime> splitSlimes;
    VECTOR position;
    VECTOR velocity;
    VECTOR dentDirection;
    float radius;
    bool grounded;
    float squashScale;
    float squashVelocity;
    float wobblePhase;
    float wobblePower;
    float droop;
    float dent;
    float dentVelocity;
    int selectedColor;
    bool oldNumberKeys[3];
    bool oldSpace;
    bool oldZ;
    int currentFace;
    int faceHandles[5];
    float faceDirectionX;
    float faceTurnX;
};
