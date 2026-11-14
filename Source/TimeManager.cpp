#include "TimeManager.h"
#include <iostream>


//*---------------------------------------------------------------------------------------
//*【?】コンストラクタ
//*----------------------------------------------------------------------------------------
TimeManager::TimeManager() :
	m_DeltaTime(0.0f),
	m_HitStopScale(1.0f),
	m_HitStopTimer(0.0f),
	m_GameElapsedTime(0.0f),
	m_TimeScale(1.0f)
{
}

//*---------------------------------------------------------------------------------------
//*【?】デストラクタ
//*----------------------------------------------------------------------------------------
TimeManager::~TimeManager()
{
}

bool TimeManager::Init()
{
	// 初期化
	m_PrevTime = std::chrono::high_resolution_clock::now();

	return true;
}

//*---------------------------------------------------------------------------------------
//*【?】更新
//*
//* [引数]なし
//* [返値]なし
//*----------------------------------------------------------------------------------------
void TimeManager::Update()
{
	// 現在の時間を取得
	auto currentTime = std::chrono::high_resolution_clock::now();

	// デルタタイムを計算 (ミリ秒単位に変換)
	auto deltaTime = currentTime - m_PrevTime;


	// 現在の時間を次のループの基準時間に更新
	m_PrevTime = currentTime;

	// デルタタイムをfloatにして保持
	float rawDeltaTime = std::chrono::duration<float>(deltaTime).count();
	m_DeltaTime = rawDeltaTime;


	if (m_HitStopTimer > 0.0f)
	{
		m_HitStopTimer -= rawDeltaTime;
		if (m_HitStopTimer <= 0.0f)
		{
			m_HitStopTimer = 0.0f;
			m_HitStopScale = 1.0f; // タイマー終了で等速に戻す
		}
	}
	else
	{
		m_HitStopScale = 1.0f;
	}

	// タイムスケール適用
	m_DeltaTime = m_DeltaTime * m_TimeScale * m_HitStopScale;


	// 経過時間を記録
	m_GameElapsedTime += m_DeltaTime;

	// --- 追加：デルタタイムの上限（Clamp）処理 ---
	// 例: 0.05秒（5fps相当の処理落ち）を上限とする
	const float MAX_DELTA_TIME = 0.05f;
	if (m_DeltaTime > MAX_DELTA_TIME)
	{
		m_DeltaTime = MAX_DELTA_TIME;
	}

	// タイマー処理更新
	TimersUpdate();
}
//*---------------------------------------------------------------------------------------
//*【?】ヒットストップ
//*
//* [引数]
//* _duration : 停止時間（秒） 
//* _scale    : 停止時間のスケール（倍率） 
//* [返値]
//* なし 
//*----------------------------------------------------------------------------------------
void TimeManager::TriggerHitStop(float _duration, float _scale)
{
	if (_duration > m_HitStopTimer)
	{
		m_HitStopTimer = _duration;
	}

	if (_scale < m_HitStopScale)
	{
		m_HitStopScale = _scale;
	}
}
//*---------------------------------------------------------------------------------------
//*【?】遅延処理用のタイマーを更新
//*
//* [引数] なし 
//* [返値] なし 
//*----------------------------------------------------------------------------------------
void TimeManager::TimersUpdate()
{
	for (auto it = m_Timers.begin(); it != m_Timers.end(); )
	{
		it->_remainingTime -= m_DeltaTime;
		if (it->_remainingTime <= 0.0f)
		{
			// タイマーが終了したらコールバックを呼び出す
			if (it->_callback)
			{
				it->_callback();
			}
			// タイマーをリストから削除
			it = m_Timers.erase(it);
		}
		else
		{
			++it;
		}
	}
}
//*---------------------------------------------------------------------------------------
//*【?】遅延処理用のタイマーを追加
//*
//* [引数]
//* _duration : 遅延時間（秒）
//* _callback : コールバック関数
//*
//* [返値]
//* なし 
//*----------------------------------------------------------------------------------------
void TimeManager::AddTimer(float _executionDelay, std::function<void()> _callback)
{
	Timer newTimer;
	newTimer._remainingTime = _executionDelay;
	newTimer._callback = _callback;

	m_Timers.push_back(newTimer);
}
