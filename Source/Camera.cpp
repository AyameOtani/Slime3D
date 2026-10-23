#include "Camera.h"
#include "Utility.h"

Camera::Camera()
    : mCameraX(0.0f),
    mStageZ(0.0f),
    mFollowRate(0.045f),
    mPosition(VGet(0.0f, 0.0f, 0.0f)),
    mTarget(VGet(0.0f, 0.0f, 0.0f)),
    m_CameraAngle(CAMERA_ANGLE::ANGLE_A)
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
    // カメラアングル切り替え
    // --------------------------------------------------------
    if (CheckHitKey(KEY_INPUT_8))
    {
        m_CameraAngle = Camera::CAMERA_ANGLE::ANGLE_A;
    }
    else if (CheckHitKey(KEY_INPUT_9))
    {
        m_CameraAngle = Camera::CAMERA_ANGLE::ANGLE_B;
    }
    else if (CheckHitKey(KEY_INPUT_0))
    {
        m_CameraAngle = Camera::CAMERA_ANGLE::ANGLE_C;
    }

    // --------------------------------------------------------
    // カメラ位置
    // --------------------------------------------------------
    VECTOR posOffset = m_CameraAngleSettingDataList[
        UINT_CAST(m_CameraAngle)].positionOffset;;
    
    mPosition = VGet( mCameraX + posOffset.x, posOffset.y, mStageZ - posOffset.z);

    // --------------------------------------------------------
    // カメラが見る場所
    //
    // プレイヤーより少し右側を見ることで
    // 進行方向を広く表示する
    // --------------------------------------------------------
    VECTOR targetOffset = m_CameraAngleSettingDataList[
        UINT_CAST(m_CameraAngle)].targetOffset;;

    mTarget = VGet(mCameraX + targetOffset.x, targetOffset.y, mStageZ + targetOffset.z);
}

//*---------------------------------------------------------------------------------------
//*【?】カメラアングル用データのセットアップ [山本]
//*
//* [引数] なし
//* [返値] なし
//*----------------------------------------------------------------------------------------
void Camera::SetupCameraAngleSettingData()
{
    /* 
    * アングルA 
    */
    m_CameraAngleSettingDataList[UINT_CAST(CAMERA_ANGLE::ANGLE_A)] =
    {
        VGet(
            -0.80f,     // カメラをプレイヤーより少し左へ  
            10.0f,      // カメラの高さ
            9.0f        // プレイヤーからカメラまでの奥行き
        ),
        VGet( 
            0.8f,       // プレイヤーより少し右側を見る
            1.15f,      // 注視点の高さ
            0.0f        // 変更なし
        ),
    };   
    
    /*
    * アングルB
    */
    m_CameraAngleSettingDataList[UINT_CAST(CAMERA_ANGLE::ANGLE_B)] =
    {
        VGet(
            -10.0f,       // カメラはプレイヤーの後ろに
            5.0f,        // カメラの高さ
            0.0f         // ステージと同じ軸にいる
        ),
        VGet(
            0.0f,       
            1.15f,      // 少し上見る
            0.0f
        ),
    };    
    
    /*
    * アングルC
    */
    m_CameraAngleSettingDataList[UINT_CAST(CAMERA_ANGLE::ANGLE_C)] =
    {
        VGet(
            -5.0f,       // カメラはプレイヤーと同じ軸にいる
            15.0f,       // カメラの高さ
            0.0f         // ステージと同じ軸にいる
        ),
        VGet(
            0.0f,
            0.15f,      // 少し上見る
            0.0f
        ),
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
