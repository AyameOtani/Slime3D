#pragma once
#include "IState.h"
#include <map>

// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:CompositeState Class --- */
//
//  ★★★抽象クラス★★★
// 
//  ★継承：IState ★
//  
// 【?】子ステートを持つ分岐ステート的な役割
//		シーンで言えば、
//			
//					タイトル(Composite)
//					   ↓
//				  ---------------
//				  ↓			↓
//		ステージ選択(IState)	装備選択(Composite)
//											↓
//									------------------
//									↓				↓
//							 装備１(IState)		装備２(IState)								
// 
// ***************************************************************************************
template<typename Owner>
class CompositeState : public IState<Owner>
{
protected:
    std::map<int, std::shared_ptr<IState<Owner>>> m_pChildStateMap; // 子ステートのリスト
    int m_CrntChildStateID;   // 現在の子ステートID

public:
	CompositeState() : m_CrntChildStateID(-1) {};
	virtual ~CompositeState() = default;

	void OnEnter(Owner* pOwner) override
	{

	}

	void OnExit(Owner *pOwner)override
	{

	}

	int Update(Owner *pOwner)override
	{
		return -1;
	}

	void Draw(Owner *pOwner)override
	{

	}


	/// <summary>
	/// 子ステートの追加
	/// </summary>
	/// <param name="id"> 子ステートのeunm </param>
	/// <param name="pOwner"></param>
	void add_Child(const int id, std::shared_ptr<IState<Owner>> pState)override 
	{
		m_pChildStateMap[id] = (pState);
	}

	/// <summary>
	/// 最初の子ステートを設定（-1の場合、何もしない）
	/// </summary>
	/// <param name="id"></param>
	void SetInitChildState(Owner *pOwner, const int id)
	{
		if (id != -1) {
			m_pChildStateMap[id]->OnEnter(pOwner);
		}
		m_CrntChildStateID = id;
	}


	/// <summary>
	/// 子ステートを切り替える
	/// </summary>
	/// <param name="pOwner"></param>
	/// <param name="id">切り替え先の子ステートのenum</param>
	void ChangeChildState(Owner *pOwner, const int newId) override
	{
		m_pChildStateMap[m_CrntChildStateID]->OnExit(pOwner);
		m_pChildStateMap[newId]->OnEnter(pOwner);
		m_CrntChildStateID = newId;
	}
};

