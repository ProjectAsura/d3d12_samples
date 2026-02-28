//-----------------------------------------------------------------------------
// File : asdxThreadPool.cpp
// Desc : Worker Thread.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cassert>
#include <fnd/asdxThreadPool.h>


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// ThreadPool class
///////////////////////////////////////////////////////////////////////////////
class ThreadPool : public IThreadPool
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    /* NOTHING */

public:
    //=========================================================================
    // public variables.
    //=========================================================================
    /* NOTHING */

    //=========================================================================
    // public methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //-------------------------------------------------------------------------
    ThreadPool(uint8_t threadCount)
    : m_RequestTerminate(false)
    {
        for(auto i=0u; i<threadCount; ++i)
        { m_Threads.emplace_back(std::thread(m_Worker)); }
    }

    //-------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //-------------------------------------------------------------------------
    ~ThreadPool()
    {
        // ブロック.
        {
            std::unique_lock<std::mutex> locker(m_Mutex);
            m_RequestTerminate = true;
        }

        m_Condtion.notify_all();

        auto count = m_Threads.size();
        for(auto i=0; i<count; ++i)
        { m_Threads.at(i).join(); }
    }

    //-------------------------------------------------------------------------
    //! @brief      解放処理を行います.
    //-------------------------------------------------------------------------
    void Release() override
    { delete this; }

    //-------------------------------------------------------------------------
    //! @brief      ジョブを追加します.
    //-------------------------------------------------------------------------
    void Push(IRunnable* runnable) override
    {
        assert(runnable != nullptr);

        // ブロック.
        {
            std::unique_lock<std::mutex> locker(m_Mutex);
            m_Queue.push(runnable);
        }

        m_Condtion.notify_all();
    }

    //-------------------------------------------------------------------------
    //! @brief      ジョブを追加します.
    //-------------------------------------------------------------------------
    void Push(uint32_t count, IRunnable** runnables) override
    {
        assert(count > 0);
        assert(runnables != nullptr);

        // ブロック.
        {
            std::unique_lock<std::mutex> locker(m_Mutex);
            for(auto i=0u; i<count; ++i)
            { m_Queue.push(runnables[i]); }
        }

        m_Condtion.notify_all();
    }

    //-------------------------------------------------------------------------
    //! @brief      ジョブの完了を待機します.
    //-------------------------------------------------------------------------
    void Wait() override
    {
        std::unique_lock<std::mutex> locker(m_Mutex);
        if (m_Queue.empty())
        { return; }

        m_Condtion.wait(locker);
    }

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    bool                        m_RequestTerminate = false;
    asdx::Queue<IRunnable>      m_Queue;
    std::mutex                  m_Mutex;
    std::condition_variable     m_Condtion;
    std::vector<std::thread>    m_Threads;

    std::function<void()> m_Worker = [this]()
    {
        while(true)
        {
            IRunnable* runnable = nullptr;
            {
                std::unique_lock<std::mutex> locker(m_Mutex);
                while(m_Queue.empty())
                {
                    if (m_RequestTerminate)
                    { return; }

                    m_Condtion.wait(locker);
                }

                runnable = m_Queue.pop();
                assert(runnable != nullptr);
            }

            runnable->Run();
        }
    };

    //=========================================================================
    // private methods.
    //=========================================================================
    /* NOTHING */
};

//-----------------------------------------------------------------------------
//      スレッドプールを生成します.
//-----------------------------------------------------------------------------
bool CreateThreadPool(uint8_t threadCount, IThreadPool** ppThreadPool)
{
    auto instance = new(std::nothrow) ThreadPool(threadCount);
    if (instance == nullptr)
    { return false; }

    *ppThreadPool = instance;
    return true;
}

} // namespacea asdx
