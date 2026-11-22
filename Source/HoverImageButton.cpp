#include "HoverImageButton.h"
#include "Mouse.h"



// 不備があったら教えてください.大谷

//*---------------------------------------------------------------------------------------
//*【?】コンストラクタ
//*---------------------------------------------------------------------------------------
HoverImageButton::HoverImageButton(float x, float y, float angle, std::string filename, float rate, float changerate)
{
	// 渡された位置、角度、拡大率などの各種パラメータをメンバ変数に保持して初期状態を構築する
	mx = x;
	my = y;
	mAngle = angle;
	mRate = rate;
	mChangeRate = changerate;
	isActive = true;
	isHover = false;

	// 指定されたファイルパスから画像を読み込み、ハンドルを取得する
	mnHandle = LoadGraph(filename.c_str());

	// 画像の読み込みに失敗した場合は、デバッグ画面にエラーメッセージを表示する
	if (mnHandle == InvalidGraphHandle)
	{
		printfDx("ホバー画像読み込み失敗");
	}
}

//*---------------------------------------------------------------------------------------
//*【?】デストラクタ
//*---------------------------------------------------------------------------------------
HoverImageButton::~HoverImageButton()
{
	if (mnHandle != InvalidGraphHandle)
	{
		DeleteGraph(mnHandle);
		mnHandle = InvalidGraphHandle;
	}
}

//*---------------------------------------------------------------------------------------
//*【?】更新処理
//*---------------------------------------------------------------------------------------
void HoverImageButton::Update()
{
	// 画像ハンドルが無効な場合は処理を行わない
	if (mnHandle == InvalidGraphHandle) return;

	int w, h;
	GetGraphSize(mnHandle, &w, &h);

	if (!isActive)
	{
		isHover = false;
		return;
	}

	// 現在の拡大率を考慮したボタンの半分の幅と高さを計算する（中心基準の判定用）
	float halfWidth = (w * mRate) / CenterDivisor;
	float halfHeight = (h * mRate) / CenterDivisor;

	// 現在のマウス座標が、拡大率を考慮したボタンの矩形範囲内に収まっているかを判定する
	if (Mouse::x >= mx - halfWidth
		&& Mouse::x <= mx + halfWidth
		&& Mouse::y >= my - halfHeight
		&& Mouse::y <= my + halfHeight)
	{
		isHover = true;  // 範囲内の場合はホバー状態を有効にする
	}
	else
	{
		isHover = false; // 範囲外の場合はホバー状態を解除する
	}
}


//*---------------------------------------------------------------------------------------
//*【?】描画処理
//*---------------------------------------------------------------------------------------
void HoverImageButton::Draw()
{
	// 画像ハンドルが無効な場合は描画を行わない
	if (mnHandle == InvalidGraphHandle) return;

	int w, h;
	GetGraphSize(mnHandle, &w, &h);

	// マウスが乗っているかどうかで適用する拡大率を切り替える
	float rate = isHover ? mChangeRate : mRate;

	// ボタンが無効な状態のときは、視覚的に分かりやすくするためにブレンドモードを設定して暗く描画する
	if (!isActive)
	{
		SetDrawBlendMode(DX_BLENDMODE_MULA, BlendModeParamMax);
	}

	// 指定された座標、拡大率、回転角度を考慮して、画像を中心基準で回転描画する
	DrawRotaGraph((int)mx, (int)my, rate, mAngle, mnHandle, TRUE);

	// 描画後はブレンドモードの設定を通常の状態に戻す
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}


//*---------------------------------------------------------------------------------------
//*【?】クリック判定
//*---------------------------------------------------------------------------------------
bool HoverImageButton::IsClicked()
{
	// マウスがボタン上に乗っていて、かつマウスのトリガーであるかを判定して返す
	return isHover && Mouse::IsTrigger();
}


//*---------------------------------------------------------------------------------------
//*【?】位置の設定
//*---------------------------------------------------------------------------------------
void HoverImageButton::SetPosition(float x, float y)
{
	mx = x;
	my = y;
}


//*---------------------------------------------------------------------------------------
//*【?】角度の設定
//*---------------------------------------------------------------------------------------
void HoverImageButton::SetAngle(float angle)
{
	mAngle = angle;
}