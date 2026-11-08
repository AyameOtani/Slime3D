#pragma once
#include <memory>

// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:IState Class --- */
//
//  ★★★抽象クラス★★★
//
// 【?】ステートの基底クラス
//		HSMを使用した設計に対応している。
//		子ステートが必要な場合は派生先のCompositeStateを継承して
//		クラスを作ることで明示的に子ステートを持っていることを表す。
//
// ***************************************************************************************
template<typename Owner>
class IState
{
protected:
	std::weak_ptr<IState<Owner>> m_pParent;	// 親ステート

public:
	IState() = default;
	virtual ~IState() = default;

	virtual void OnEnter(Owner *pOwner) = 0;
	virtual void OnExit(Owner* pOwner) = 0;
	virtual int Update(Owner *pOwner) = 0;
	virtual void Draw(Owner *pOwner) {};	// 必ずしも描画処理が入るとは限らないので仮想関数にする

	/// <summary>
	/// 子ステートの追加
	/// </summary>
	/// <param name="id"> 子ステートのeunm </param>
	/// <param name="pOwner"></param>
	virtual void add_Child(const int id, std::shared_ptr<IState<Owner>> pState) {};


	/// <summary>
	/// 親ステートを設定
	/// </summary>
	/// <param name="pState"></param>
	virtual void set_Parent(std::shared_ptr<IState<Owner>> pState) 
	{
		m_pParent = pState;
	};

	/// <summary>
	/// 子ステートを切り替える
	/// </summary>
	/// <param name="pOwner"></param>
	/// <param name="id">切り替え先の子ステートのenum</param>
	virtual void ChangeChildState(Owner *pOwner, const int id) {};
};

