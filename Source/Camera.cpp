#include "Camera.h"
#include "Utility.h"

Camera::Camera()
    : mCameraX(0.0f),
    mStageZ(0.0f),
    mFollowRate(0.045f),
    mPosition(VGet(0.0f, 0.0f, 0.0f)),
    mTarget(VGet(0.0f, 0.0f, 0.0f))
{
}

void Camera::Initialize(
    const VECTOR& playerPosition)
{
    // カメラアングルデータのセットアップ
    SetupCameraAngleSettingData();

    // 初期位置をプレイヤー位置に合わせる
    mCameraX = playerPosition.x;

    // 横スクロールなので
    // Z方向は開始位置を基準に固定する
    mStageZ = playerPosition.z;

    // 横スクロール向けに少し狭めの視野角
    SetupCamera_Perspective( 43.0f * DX_PI_F / 180.0f );

    // 現在のゲームは1～10程度の座標なので
    // 描画範囲もそれに合わせる
    SetCameraNearFar( 0.1f, 100.0f);

    RebuildCamera();
    Apply();
}

void Camera::Update( const VECTOR& playerPosition)
{
    // プレイヤーのX位置へ少しずつ近づける
    //
    // すぐに同じ位置へ移動させず、
    // 遅れて追いかけることで
    // 横スクロールゲームらしい滑らかなカメラになる
    mCameraX += (playerPosition.x - mCameraX) * mFollowRate;

    RebuildCamera();
    Apply();
}

void Camera::RebuildCamera()
{
    // --------------------------------------------------------
    // カメラ位置
    // --------------------------------------------------------
    mPosition = VGet( mCameraX + CAMERA_OFFSET_X, CAMERA_Y, mStageZ - CAMERA_DISTANCE_Z );

    // --------------------------------------------------------
    // カメラが見る場所
    //
    // プレイヤーより少し右側を見ることで
    // 進行方向を広く表示する
    // --------------------------------------------------------
    mTarget = VGet( mCameraX + TARGET_OFFSET_X, TARGET_Y, mStageZ );
}

//*---------------------------------------------------------------------------------------
//*【?】カメラアングル用データのセットアップ [山本]
//*
//* [引数] なし
//* [返値] なし
//*----------------------------------------------------------------------------------------
void Camera::SetupCameraAngleSettingData()
{
    m_CameraAngleSettingDataList[UINT_CAST(CAMERA_ANGLE::ANGLE_A)] =
    {
        VGet(-0.80f, 10.0f, 9.0f),
        VGet( 0.8f,  1.15f, 0.0f),
    };   
    
    m_CameraAngleSettingDataList[UINT_CAST(CAMERA_ANGLE::ANGLE_B)] =
    {
        VGet(-0.80f, 10.0f, 0.0f),
        VGet(0.8f,  1.15f, 0.0f),
    };    
    
    m_CameraAngleSettingDataList[UINT_CAST(CAMERA_ANGLE::ANGLE_C)] =
    {
        VGet(-0.80f, 10.0f, 0.0f),
        VGet(0.8f,  1.15f, 0.0f),
    };
}


void Camera::Apply() const
{
    SetCameraPositionAndTarget_UpVecY( mPosition, mTarget );
}

void Camera::SetFollowRate( float rate)
{
    if (rate < 0.0f)
    {
        rate = 0.0f;
    }

    if (rate > 1.0f)
    {
        rate = 1.0f;
    }

    mFollowRate = rate;
}

VECTOR Camera::GetPosition() const
{
    return mPosition;
}

VECTOR Camera::GetTarget() const
{
    return mTarget;
}
