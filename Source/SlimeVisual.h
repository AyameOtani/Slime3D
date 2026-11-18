#pragma once
#include "Dxlib.h"
#include <vector>
#include "GameDefinitions.h"

class SlimeObject;

// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:Slime Class --- */
//
// 【?】スライムの見た目部分の処理を行う
//		変形や表情の変化など
//
// ***************************************************************************************
class SlimeVisual
{
private:
    struct Mesh
    {
        std::vector<VERTEX3D> vertices;      // 頂点
        std::vector<unsigned short> indices; // 頂点インデックス
    };
	static const int LAT = 12;
	static const int LON = 20;

    // 位置・大きさ・揺れを反映する描画用
    Mesh m_Body;
    Mesh m_Eyes;

    // 子供の変形前の形状（毎回ここから描画頂点を作る）
    std::vector<VERTEX3D> m_ChildBodySource;    
    std::vector<VERTEX3D> m_ChildEyeSource;


    bool m_IsRainbow = true;
    bool m_MeshReady = false;
    int m_BuiltColor = 1;

    int m_VisualAge = 0;
    //float m_ChildPhase = 0.0f;
    VECTOR velocity;
    VECTOR dentDirection;
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


public:
	SlimeVisual();
	~SlimeVisual();

    void Setup();
	void Update(const SlimeObject& slime, float deltaTime);
    void RefreshMesh(const SlimeObject& slime); // 時間を進めず頂点を作り直す
    void Draw(const SlimeObject& slime) ;
    void DrawShadow() ;

	void Reset();

    void SetFace(int face);         // 表情の設定
    void OnJump();  
    void OnLand(float impactSpeed);
    void OnWallContact(
        const VECTOR& direction,
        float penetration,
        float impactSpeed);

private:
    void EnsureMesh(const SlimeObject& slime);
    void CreateRainbowMesh();
    void CreateChildMesh(int color);

    void UpdateDeformation(const SlimeObject& slime, float deltaTime);
    void UpdateRainbowMesh(const SlimeObject& slime);
    void UpdateChildMesh(const SlimeObject& slime);
    void UpdateFaceDirection(const SlimeObject& slime);
    void DrawRainbowFace(const SlimeObject& slime) const;

    void DrawMesh(const Mesh& mesh);

    float GetSupportAmount(float x, float z, float centerY, float r) const;
    bool HasSupport(float x, float z, float centerY, float r) const;

};

