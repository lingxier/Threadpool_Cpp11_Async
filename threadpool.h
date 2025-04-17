#pragma once
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <map>
#include <future>
using namespace std;

// œﬂ≥Ã≥ÿ¿‡
class ThreadPool
{
public:
    ThreadPool(int min = 4, int max = thread::hardware_concurrency());
    ~ThreadPool();
    void addTask(function<void()> f);
    template<typename F, typename... Args>
    auto addTask(F&& f, Args&&... args) -> future<typename result_of<F(Args...)>::type>
    {
        using returnType = typename result_of<F(Args...)>::type;
        auto task = make_shared<packaged_task<returnType()>>(
            bind(forward<F>(f), forward<Args>(args)...)
        );
        future<returnType> res = task->get_future();
        {
            unique_lock<mutex> lock(m_queueMutex);
            m_tasks.emplace([task]() { (*task)(); });
        }
        m_condition.notify_one();
        return res;
    }

private:
    void manager();
    void worker();
private:
    thread* m_manager;
    map<thread::id, thread> m_workers;
    vector<thread::id> m_ids;
    int m_minThreads;
    int m_maxThreads;
    atomic<bool> m_stop;
    atomic<int> m_curThreads;
    atomic<int> m_idleThreads;
    atomic<int> m_exitNumber;
    queue<function<void()>> m_tasks;
    mutex m_idsMutex;
    mutex m_queueMutex;
    condition_variable m_condition;
};