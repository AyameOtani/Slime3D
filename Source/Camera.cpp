#include "Camera.h"
#include "Utility.h"

namespace
{
    constexpr float TARGET_OFFSET_Y = 1.25f;    // 注視点の位置を少し上にずらす補正
    constexpr float PITCH_CLAMP_MAX = 80.0f;    // 上下の向きの制限 最大角度
    constexpr float PITCH_CLAMP_MIN = -80.0f;   // 上下の向きの制限 最小角度
}

//*---------------------------------------------------------------------------------------
//*【?】コンストラクタ
//*----------------------------------------------------------------------------------------
Camera::Camera()
    : mCameraX(0.0f)
    , mStageZ(0.0f)
    , mFollowRate(0.045f)
    , mPosition(VGet(0.0f, 0.0f, 0.0f))
    , mTarget(VGet(0.0f, 0.0f, 0.0f))
    , m_CameraNear(0.1f)
	, m_CameraFar(100.0f)
    , m_CameraFovRad(Helper::Math::DegToRad(60.0f))
    , m_CameraAngle(CAMERA_ANGLE::ANGLE_A)
    , m_CameraAngleSettingDataList()
    , m_ControlMode(CONTROL_MODE::FIXED)
    , m_Yaw(0.0f)
    , m_Pitch(Helper::Math::DegToRad(-20.0f))
    , m_TpsDistance(10.0f)
    , m_MouseSensitivity(0.003f)
    , m_MouseLookReady(false)
{
}

//*---------------------------------------------------------------------------------------
//*【?】初期化処理
//*
//* [引数] 
//* playerPosition : プレイヤーの位置
//* 
//* [返値] なし
//*----------------------------------------------------------------------------------------
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

    // マウスの表示を消す
    SetMouseDispFlag(FALSE);

    RebuildCamera();
    Apply();
}

//*---------------------------------------------------------------------------------------
//*【?】更新処理
//*
//* [引数] 
//* playerPosition : プレイヤーの位置
//* 
//* [返値] なし
//*----------------------------------------------------------------------------------------
void Camera::Update( const VECTOR& playerPosition)
{
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;

    if (m_ControlMode == CONTROL_MODE::TPS &&
        GetWindowActiveFlag() != FALSE)
    {
        // 中央
        const int centerX = Utility::SCREEN_WIDTH / 2;
        const int centerY = Utility::SCREEN_HEIGHT / 2;

        int mouseX = 0;
        int mouseY = 0;

        if (GetMousePoint(&mouseX, &mouseY) == 0)
        {
            // 初回は移動量を使わず、中央へ戻すだけにする
            if (m_MouseLookReady)
            {
                mouseDeltaX = static_cast<float>(mouseX - centerX);
                mouseDeltaY = static_cast<float>(mouseY - centerY);
            }

            // 次のフレームの移動量を測るため、中央へ戻す
            m_MouseLookReady =
                SetMousePoint(centerX, centerY) == 0;
        }
        else
        {
            m_MouseLookReady = false;
        }
    }
    else
    {
        m_MouseLookReady = false;
    }


    // 固定アングル用の追従位置は更新し続ける
    // TPSから戻したときも、現在のX位置を追従できる
    mCameraX += (playerPosition.x - mCameraX) * mFollowRate;

    if (m_ControlMode == CONTROL_MODE::TPS)
    {
        RebuildTpsCamera(playerPosition, mouseDeltaX, mouseDeltaY);
    }
    else
    {
        // 現在の固定アングル処理
        RebuildCamera();
    }

    // どちらのモードも、現在の反映処理を使う
    Apply();
}

//*---------------------------------------------------------------------------------------
//*【?】カメラ情報の構築
//*
//* [引数] なし
//* [返値] なし
//*----------------------------------------------------------------------------------------
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
//*【?】TPS操作用のカメラ情報を構築
//*
//* [引数] なし
//* [返値] なし
//*----------------------------------------------------------------------------------------
void Camera::RebuildTpsCamera(const VECTOR& playerPosition, float mouseDeltaX, float mouseDeltaY)
{
    // 左右の向き
    // 角度が増え続けないよう、-180～180度の間に制限する
    m_Yaw = std::remainder(
        m_Yaw + mouseDeltaX * m_MouseSensitivity,
        2.0f * DX_PI_F
    );

    m_Pitch -= mouseDeltaY * m_MouseSensitivity;

    // 上下の向きを制限する
    m_Pitch = Helper::Math::Clamp(
        m_Pitch,
        Helper::Math::DegToRad(PITCH_CLAMP_MIN),
        Helper::Math::DegToRad(PITCH_CLAMP_MAX)
    );

    // 注視点の位置を少し上にする
    mTarget = VAdd(playerPosition, VGet(0.0f, TARGET_OFFSET_Y, 0.0f));

    // カメラの見ている方向
    const float cosPitch = std::cos(m_Pitch);
    const VECTOR foward = VGet(
        std::sin(m_Yaw) * cosPitch,
        std::sin(m_Pitch),
        std::cos(m_Yaw) * cosPitch
    );

    // 位置
    mPosition = VSub(mTarget, VScale(foward, m_TpsDistance));
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
