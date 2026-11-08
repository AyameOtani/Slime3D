#pragma once
#include "IState.h"
#include <map>

// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:StateMachine Class --- */
//
// 【?】ステートの管理を行う
//		ステートパターンを使用したい各クラスにこのStateMachineを持たせ、
//		ステートをセットすることで使用できる。
//
// ***************************************************************************************
template<typename Owner>
class StateMachine
{
private:
	Owner *m_pOwner;  // このステートマシンの持ち主
	int m_CrntStateID;    // 現在のステートID

	// 連想配列で管理
	std::map<int, std::shared_ptr<IState<Owner>>> m_pStateMap;

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="pOwner"></param>
	StateMachine(Owner *pOwner) :
		m_pOwner(pOwner),
		m_CrntStateID(-1)
	{
	}

	/// <summary>
	/// デストラクタ
	/// </summary>
	~StateMachine()
	{
		m_pStateMap.clear();
	}

	int get_CrntStateIndex()const { return m_CrntStateID; }

	/// <summary>
	/// ステートを設定する
	/// </summary>
	/// <param name="id"></param>
	/// <param name="pState"></param>
	void RegisterState(const int id, std::shared_ptr<IState<Owner>> pState)
	{
		m_pStateMap[id] = pState;
	}


	/// <summary>
	/// 現在のステートを設定
	/// </summary>
	/// <param name="id"></param>
	void SetStartState(const int id)
	{
		m_CrntStateID = id;
		m_pStateMap[id]->OnEnter(m_pOwner);
	}

	/// <summary>
	/// ステートを変更する
	/// </summary>
	/// <param name="pState"></param>
	void ChangeState(const int newId)
	{
		m_pStateMap[m_CrntStateID]->OnExit(m_pOwner);
		m_pStateMap[newId]->OnEnter(m_pOwner);
		m_CrntStateID = newId;
	}

	/// <summary>
	/// 更新
	/// </summary>
	void Update()
	{
		if (m_CrntStateID == -1)return;

		if (m_pStateMap[m_CrntStateID]) {
			int newState = m_pStateMap[m_CrntStateID]->Update(m_pOwner);

			// ステートが変更されていたら切り替え処理を行う
			if (newState != m_CrntStateID)
			{
				ChangeState(newState);
			}
		}
	}

	/// <summary>
	/// 描画
	/// </summary>
	void Draw()
	{
		if (m_CrntStateID == -1)return ;

		if (m_pStateMap[m_CrntStateID]) {
			m_pStateMap[m_CrntStateID]->Draw(m_pOwner);
		}
	}
};

