#include "Camera.h"
#include "Utility.h"

Camera::Camera()
    : mCameraX(0.0f),
    mStageZ(0.0f),
    mFollowRate(0.045f),
    mPosition(VGet(0.0f, 0.0f, 0.0f)),
    mTarget(VGet(0.0f, 0.0f, 0.0f)),
    m_CameraNear(0.1f),
	m_CameraFar(100.0f),
    m_CameraFovRad(Helper::Math::DegToRad(60.0f)),
    m_CameraAngle(CAMERA_ANGLE::ANGLE_A),
    m_CameraAngleSettingDataList()
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
    SetupCamera_Perspective(m_CameraFovRad);

    // 現在のゲームは1～10程度の座標なので
    // 描画範囲もそれに合わせる
    SetCameraNearFar(m_CameraNear, m_CameraFar);

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

//*---------------------------------------------------------------------------------------
//*【?】カメラ設定の適用
//*
//* [引数] なし
//* [返値] なし
//*----------------------------------------------------------------------------------------
void Camera::Apply() const
{
    // カメラの視野角を設定する
    SetupCamera_Perspective(m_CameraFovRad);

    // カメラのクリッピング距離を設定する
    SetCameraNearFar(m_CameraNear, m_CameraFar);

	// カメラの位置と注視点を設定する
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
