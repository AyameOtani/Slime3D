#pragma once
#include <vector>
#include <functional>

// ***************************************************************************************
// ---------------------------------------------------------------------------------------
/* --- @:ObjectPool Class --- */
//
// 【?】オブジェクトプール
//      ObjectManagerがオブジェクトの生成、破棄を行うため、このクラスでは
//      あくまでプールの仕組み飲み提供して、生成破棄などは行わない。
//      生ポインタで扱っているのは、shared_ptrでObjectManagerから渡されるようにしてしまうと、
//      アクセスにオーバーヘッドが生じてしまい、プールの大量オブジェクトの管理という利点をそがれてしまうため。
//
// ***************************************************************************************
template <typename T>
class ObjectPool 
{
private:
    std::vector<T*> m_pObjects;              // プール配列
    std::function<void (T*)> m_pGetFunc;     // 取得時に実行するメソッド   
    std::function<void (T*)> m_pReleaseFunc; // 返却時に実行するメソッド
    std::function<T* (void)> m_pCreateFunc;  // プールが足りなくなった際に実行するメソッド
    int m_MaxPoolSize;                       // 最大プール数
    int m_CreateCounter;                     // 生成カウンタ
public:
    ObjectPool(
        std::function<void (T*)> _getFunc,          // オブジェクトの取得時に実行  戻り値：void 引数：T*
        std::function<void (T*)> _releaseFunc,      // オブジェクトの返却時に実行  戻り値：void 引数：T*
        std::function<T* (void)> _createFunc,       // オブジェクトの生成時に実行  戻り値：T*   引数：void
        int _defaultCapacity,                       // デフォルトのプールサイズ
        int _maxPoolSize)                           // プールの最大数
        :
        m_pGetFunc(_getFunc),
        m_pReleaseFunc(_releaseFunc),
        m_pCreateFunc(_createFunc),
        m_MaxPoolSize(_maxPoolSize),
        m_CreateCounter(0)
    {
        m_pObjects.reserve(m_MaxPoolSize);

        // デフォルト分生成
        for (int i = 0; i < _defaultCapacity; i++)
        {
            auto obj = m_pCreateFunc();
            m_pObjects.push_back(obj);

            m_CreateCounter++;
        }
    };

    ~ObjectPool()
    {
        m_pObjects.clear();
    };

    /// <summary>
    /// オブジェクトの取得
    /// 無ければ生成（最大数を越えていたらぬるぽ）
    /// </summary>
    /// <returns></returns>
    T *get()
    {
        T *obj = nullptr;

        if (m_pObjects.empty()) 
        {
            // プールの最大数を越えていたら、ぬるぽ
            if (m_CreateCounter >= m_MaxPoolSize)
            {
                return nullptr;
            }

            // 足りない場合は生成
            obj = m_pCreateFunc();
            if (obj == nullptr) {
                return nullptr;
            }

            // 生成カウンタ加算
            m_CreateCounter++;  
        }
        else 
        {
            // 後ろからオブジェクトを取得し、プールから削除する
            obj = m_pObjects.back();
            m_pObjects.pop_back();
        }

        m_pGetFunc(obj);    // 取得時のメソッド実行
        return obj;
    }
    
    /// <summary>
    /// 使用が終わったオブジェクトをプールへ戻す
    /// </summary>
    /// <param name="obj"></param>
    void release(T *obj)
    {
        if (obj == nullptr)return;

        // 返却時のメソッド実行
        m_pReleaseFunc(obj);

        m_pObjects.push_back(obj);
    }

    /// <summary>
    /// プールを空にする（所有権を持っていないので、中身をクリアするだけ）
    /// シーン終了時やマネージャが全削除した際に呼ぶ
    /// </summary>
    void clear()
    {
        m_pObjects.clear();
        m_CreateCounter = 0; 
    }


    /// <summary>
    /// 最大生成数の取得
    /// </summary>
    /// <returns></returns>
    int get_MaxNum()const { return m_MaxPoolSize; };


    /// <summary>
    /// 現在の生成数の取得
    /// </summary>
    /// <returns></returns>
    int get_CrntCreateNum()const { return m_CreateCounter; }
};
