// Threadpool_Cpp11_Async.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "threadpool.h"
#include <iostream>

ThreadPool::ThreadPool(int min, int max) : m_maxThreads(max),
m_minThreads(min), m_stop(false), m_exitNumber(0)
{
    //m_idleThreads = m_curThreads = max / 2;
    m_idleThreads = m_curThreads = min;
    cout << "线程数量: " << m_curThreads << endl;
    m_manager = new thread(&ThreadPool::manager, this);
    for (int i = 0; i < m_curThreads; ++i)
    {
        thread t(&ThreadPool::worker, this);
        m_workers.insert(make_pair(t.get_id(), move(t)));
    }
}

ThreadPool::~ThreadPool()
{
    m_stop = true;
    m_condition.notify_all();
    for (auto& it : m_workers)
    {
        thread& t = it.second;
        if (t.joinable())
        {
            cout << "**** **** 线程 " << t.get_id() << " 将要退出了..." << endl;
            t.join();
        }
    }
    if (m_manager->joinable())
    {
        m_manager->join();
    }
    delete m_manager;
}

void ThreadPool::addTask(function<void()> f)
{
    {
        lock_guard<mutex> locker(m_queueMutex);
        m_tasks.emplace(f);
    }
    m_condition.notify_one();
}

void ThreadPool::manager()
{
    while (!m_stop.load())
    {
        this_thread::sleep_for(chrono::seconds(2));
        int idle = m_idleThreads.load();
        int current = m_curThreads.load();
        if (idle > current / 2 && current > m_minThreads)
        {
            m_exitNumber.store(2);
            m_condition.notify_all();
            unique_lock<mutex> lck(m_idsMutex);
            for (const auto& id : m_ids)
            {
                auto it = m_workers.find(id);
                if (it != m_workers.end())
                {
                    cout << "######## 线程 " << (*it).first << "被销毁了...." << endl;
                    (*it).second.join();
                    m_workers.erase(it);
                }
            }
            m_ids.clear();
        }
        else if (idle == 0 && current < m_maxThreads)
        {
            thread t(&ThreadPool::worker, this);
            cout << "++++ ++++ 添加了一个线程, id: " << t.get_id() << endl;
            m_workers.insert(make_pair(t.get_id(), move(t)));
            m_curThreads++;
            m_idleThreads++;
        }
    }
}

void ThreadPool::worker()
{
    while (!m_stop.load())
    {
        function<void()> task = nullptr;
        {
            unique_lock<mutex> locker(m_queueMutex);
            while (!m_stop && m_tasks.empty())
            {
                m_condition.wait(locker);
                if (m_exitNumber.load() > 0)
                {
                    cout << "---- ---- 线程任务结束, ID: " << this_thread::get_id() << endl;
                    m_exitNumber--;
                    m_curThreads--;
                    m_idleThreads--;
                    unique_lock<mutex> lck(m_idsMutex);
                    m_ids.emplace_back(this_thread::get_id());
                    return;
                }
            }

            if (!m_tasks.empty())
            {
                cout << "取出一个任务..." << endl;
                task = move(m_tasks.front());
                m_tasks.pop();
            }
        }

        if (task)
        {
            m_idleThreads--;
            task();
            m_idleThreads++;
        }
    }
}

#if 0
void calc(int x, int y)
{
    int res = x + y;
    cout << "res = " << res << endl;
    this_thread::sleep_for(chrono::seconds(2));
}

int main()
{
    ThreadPool pool(4);
    for (int i = 0; i < 10; ++i)
    {
        auto func = bind(calc, i, i * 2);
        pool.addTask(func);
    }
    getchar();
    return 0;
}
#else
int calc(int x, int y)
{
    int res = x + y;
    //cout << "res = " << res << endl;
    this_thread::sleep_for(chrono::seconds(2));
    return res;
}

int main()
{
    ThreadPool pool(4);
    vector<future<int>> results;

    for (int i = 0; i < 10; ++i)
    {
        results.emplace_back(pool.addTask(calc, i, i * 2));
    }

    // 等待并打印结果
    for (auto&& res : results)
    {
        cout << "线程函数返回值: " << res.get() << endl;
    }

    return 0;
}
#endif



// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
