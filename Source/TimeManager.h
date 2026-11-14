#pragma once
#include <chrono>
#include <functional>

/// <summary>
/// 遅延処理を行うためのタスク
///</summary>
struct ScheduledTask
{
	float _remainingTime;				// 残り時間
	std::function<void()> _callback;	// コールバック関数
};


// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:TimeManager Class --- */
//
//  ★★★シングルトン★★★
//
// 【?】時間関係の管理
// 
// ***************************************************************************************
class TimeManager
{
private:
	float m_DeltaTime;		// デルタタイム
	float m_TimeScale;		// 現在のタイムスケール（倍率）※スローモーションとか作れます
	float m_HitStopTimer;	// 停止時間用タイマー
	float m_HitStopScale;	// 停止時間のスケール（倍率）
	float m_GameElapsedTime;// ゲーム開始時からの経過時間
	std::chrono::steady_clock::time_point m_PrevTime;	// 前回時間の保持用
	std::vector<ScheduledTask> m_ScheduledTasks;		// タイマー（遅延処理）のリスト 

public:
	TimeManager();
	~TimeManager();

	bool Init();
	void Update();

	float get_DeltaTime()const { return m_DeltaTime; }			// デルタタイムの取得
	float get_TimeScale()const { return m_TimeScale; }
	float get_GameElapsedTime()const { return m_GameElapsedTime; }

	void set_TimeScale(float _scale) { m_TimeScale = _scale; }	// タイムスケール（倍率）の設定

	void TriggerHitStop(float _duration, float _scale = 0.5f);
	
	// 遅延処理用のタスクを登録
	void RegisterScheduleTask(float _executionDelay, std::function<void()> _callback);


private:
	// コピー禁止
	TimeManager(const TimeManager&) = delete;
	TimeManager& operator=(const TimeManager&) = delete;
	// ------------------------------------------------------

	void ScheduleTaskUpdate();
};

