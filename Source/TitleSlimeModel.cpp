#include "TitleSlimeModel.h"
#include "Stage.h"

//*---------------------------------------------------------------------------------------
//*【?】コンストラクタ
//*
//* スライムの初期位置やジャンプ、つぶれ具合などの初期値を設定する
//*----------------------------------------------------------------------------------------
TitleSlimeModel::TitleSlimeModel()
    : position(VGet(0.0f, 0.0f, 0.0f))
    , groundY(0.0f)
    , jumpHeight(0.0f)
    , jumpVelocity(0.0f)
    , jumpTimer(TitleSlimeConfig::kJumpIntervalFrames)
    , modelScale(TitleSlimeConfig::kDefaultScale)
    , squashScale(TitleSlimeConfig::kSquashRest)
    , squashVelocity(0.0f)
    , initialized(false)
    , mFaceHandle(-1)
{
}

//*---------------------------------------------------------------------------------------
//*【?】デストラクタ
//*
//* 読み込んだ顔画像を解放する
//*----------------------------------------------------------------------------------------
TitleSlimeModel::~TitleSlimeModel()
{
    // 顔画像が読み込まれている場合は解放する
    if (mFaceHandle >= 0)
    {
        DeleteGraph(mFaceHandle);
    }
}

//*---------------------------------------------------------------------------------------
//*【?】スライムの初期化
//*
//* [引数] なし
//* [返値] なし
//*----------------------------------------------------------------------------------------
void TitleSlimeModel::Initialize()
{
    // スライムのメッシュを初期化する
    mSlimeMesh.Initialize();

    // 現在位置を接地位置の基準にする
    groundY = position.y;

    // ジャンプ関連の初期値を設定する
    jumpHeight = 0.0f;
    jumpVelocity = 0.0f;
    jumpTimer = TitleSlimeConfig::kJumpIntervalFrames;

    // つぶれ具合を初期状態に戻す
    squashScale = TitleSlimeConfig::kSquashRest;
    squashVelocity = 0.0f;

    // --------------------------------------------------------
    // 顔画像の読み込み
    // --------------------------------------------------------

    // 再初期化時に古い画像が残らないように解放する
    if (mFaceHandle >= 0)
    {
        DeleteGraph(mFaceHandle);
    }

    // 顔画像を読み込む
    mFaceHandle = LoadGraph(TitleSlimeConfig::kFaceImagePath);

    // 初期化完了
    initialized = true;
}

//*---------------------------------------------------------------------------------------
//*【?】スライムの更新
//*
//* [引数] なし
//* [返値] なし
//*----------------------------------------------------------------------------------------
void TitleSlimeModel::Update()
{
    // 初期化されていない場合は処理しない
    if (!initialized) return;

    // ジャンプとつぶれ具合を更新する
    UpdateJump();
}

//*---------------------------------------------------------------------------------------
//*【?】地面の高さを更新
//*
//* [引数] なし
//* [返値] なし
//*----------------------------------------------------------------------------------------
void TitleSlimeModel::UpdateGroundHeight()
{
    // ステージが無い場合は現在位置の高さを接地位置とする
    groundY = position.y;

    // 現在のステージを取得する
    Stage* stage = Stage::Active();

    // ステージが存在しない場合は終了する
    if (stage == nullptr) return;

    // スライムの位置を基準に床を検索する
    float floorY = 0.0f;
    if (stage->FindFloor(position.x, position.z,
        position.y - TitleSlimeConfig::kFloorSearchDown,
        position.y + TitleSlimeConfig::kFloorSearchUp,
        floorY))
    {
        // 床が見つかった場合は床の高さを接地位置に設定する
        groundY = floorY;
    }
}

//*---------------------------------------------------------------------------------------
//*【?】ジャンプとつぶれ具合の更新
//*
//* [引数] なし
//* [返値] なし
//*----------------------------------------------------------------------------------------
void TitleSlimeModel::UpdateJump()
{
    // --------------------------------------------------------
    // 接地判定
    // --------------------------------------------------------

    // 高さと速度がともに0以下の場合は接地していると判定する
    const bool grounded = jumpHeight <= 0.0f && jumpVelocity <= 0.0f;

    // 接地中の場合は地面の高さを更新する
    if (grounded)
    {
        UpdateGroundHeight();
    }

    // --------------------------------------------------------
    // ジャンプタイマー
    // --------------------------------------------------------

    // タイマーを毎フレーム減らす
    // 接地中にタイマーが0以下になった場合のみジャンプする
    --jumpTimer;

    if (grounded && jumpTimer <= 0)
    {
        // ジャンプ開始時の上方向の速度を設定する
        jumpVelocity = TitleSlimeConfig::kJumpStartVelocity;

        // ジャンプ開始時に体を少しつぶす
        squashVelocity -= TitleSlimeConfig::kJumpStartSquash;

        // 次のジャンプまでのタイマーをリセットする
        jumpTimer = TitleSlimeConfig::kJumpIntervalFrames;
    }

    // --------------------------------------------------------
    // ジャンプ処理
    // --------------------------------------------------------

    // 重力を加えて上昇速度を徐々に減少させる
    jumpVelocity -= TitleSlimeConfig::kGravity;

    // 現在の速度を高さに加算する
    jumpHeight += jumpVelocity;

    // --------------------------------------------------------
    // 着地判定
    // --------------------------------------------------------

    // 地面より下に移動した場合は着地したと判定する
    if (jumpHeight < 0.0f)
    {
        // 高さと速度を初期状態に戻す
        jumpHeight = 0.0f;
        jumpVelocity = 0.0f;

        // 着地時に体をつぶす
        squashVelocity -= TitleSlimeConfig::kLandingSquash;
    }

    // --------------------------------------------------------
    // つぶれ・伸びのばね処理
    // --------------------------------------------------------

    // 元の形状に戻ろうとする力を加える
    squashVelocity += (TitleSlimeConfig::kSquashRest - squashScale) * TitleSlimeConfig::kSquashSpring;

    // 振動が大きくなりすぎないように速度を減衰させる
    squashVelocity *= TitleSlimeConfig::kSquashDamping;

    // 速度をつぶれ具合に反映する
    squashScale += squashVelocity;

    // つぶれ具合が最小値を下回らないようにする
    if (squashScale < TitleSlimeConfig::kSquashMin)
        squashScale = TitleSlimeConfig::kSquashMin;

    // つぶれ具合が最大値を超えないようにする
    if (squashScale > TitleSlimeConfig::kSquashMax)
        squashScale = TitleSlimeConfig::kSquashMax;
}

//*---------------------------------------------------------------------------------------
//*【?】スライムの描画
//*
//* [引数] なし
//* [返値] なし
//*----------------------------------------------------------------------------------------
void TitleSlimeModel::Draw()
{
    // 初期化されていない場合は描画しない
    if (!initialized) return;

    // --------------------------------------------------------
    // 描画位置とサイズの計算
    // --------------------------------------------------------

    // 本編のスライムの基本半径に倍率を掛けて表示サイズを求める
    const float radius = SlimeMesh::DEFAULT_RADIUS * modelScale;

    // メッシュの底面が接地位置に合うように中心位置を計算する
    // ジャンプ中はジャンプの高さ分だけ上に移動させる
    const VECTOR drawPosition = VGet(position.x, groundY + radius + jumpHeight, position.z);

    // 現在の位置、サイズ、つぶれ具合をメッシュに反映する
    mSlimeMesh.BuildBaseShape(drawPosition, radius, squashScale);

    // ブレンドモードを通常の描画に設定する
    DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND, TitleSlimeConfig::kBlendAlphaUnused);

    // スライムの本体を描画する
    mSlimeMesh.Draw();

    // 本体の上に顔を描画する
    DrawFace(drawPosition, radius);
}

//*---------------------------------------------------------------------------------------
//*【?】スライムの顔を描画
//*
//* [引数]
//* drawPosition : スライムの描画位置
//* radius       : スライムの半径
//*
//* [返値] なし
//*----------------------------------------------------------------------------------------
void TitleSlimeModel::DrawFace(const VECTOR& drawPosition, float radius)
{
    // 顔画像が読み込まれていない場合は描画しない
    if (mFaceHandle < 0) return;

    // --------------------------------------------------------
    // カメラの方向を取得
    // --------------------------------------------------------

    // スライムの位置からカメラの位置への方向ベクトルを求める
    VECTOR toCamera = VSub(GetCameraPosition(), drawPosition);

    // 上下方向の傾きをなくし、水平方向だけを向くようにする
    toCamera.y = 0.0f;

    // カメラの方向に合わせて顔を描画する
    mSlimeMesh.DrawFace(mFaceHandle, drawPosition, radius, toCamera);
}

//*---------------------------------------------------------------------------------------
//*【?】スライムの位置を設定
//*
//* [引数]
//* pos : 設定する位置
//*
//* [返値] なし
//*----------------------------------------------------------------------------------------
void TitleSlimeModel::SetPosition(const VECTOR& pos)
{
    // スライムの位置を設定する
    position = pos;

    // 接地位置の高さも更新する
    groundY = pos.y;
}

//*---------------------------------------------------------------------------------------
//*【?】スライムの大きさを設定
//*
//* [引数]
//* scale : 設定する拡大率
//*
//* [返値] なし
//*----------------------------------------------------------------------------------------
void TitleSlimeModel::SetScale(float scale)
{
    // スライムの表示倍率を設定する
    modelScale = scale;
}