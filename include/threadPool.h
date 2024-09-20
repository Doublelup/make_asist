#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class threadPool{
    public:
        struct Task{
            int priority;
            int order;
            std::function<void()> task;
            Task(int priority, int order, std::function<void()> task): priority{priority}, order{order}, task{task} {};
            friend bool operator< (const Task& t1, const Task& t2) {return t1.priority == t2.priority ? t1.order > t2.order : t1.priority < t2.priority;}
        };
        threadPool();
        void init(size_t maxnum);
        template<typename F, typename... Args>
        auto enqueue(int p, int order, F&& f, Args&&... args)
            -> std::future<typename std::result_of<F(Args...)>::type>;
        ~threadPool();
    private:
        std::vector<std::thread> workers;
        std::priority_queue<Task> tasks;
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;
};
inline
threadPool::threadPool()
:   stop{false}
{
}

inline
void
threadPool::init(size_t maxnum)
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop)
            throw std::runtime_error("Init an stopped threadPool.");
        for (size_t i = 0; i < maxnum; ++i)
            workers.emplace_back(
                [this]{
                    for(;;)
                    {
                        std::function<void()> task;

                        {
                            std::unique_lock<std::mutex> lock(this->queue_mutex);
                            this->condition.wait(lock,
                                [this]{return this->stop || !this->tasks.empty();});
                            if (this->stop && this->tasks.empty())
                                return;
                            task = std::move(this->tasks.top().task);
                            this->tasks.pop();
                        }

                        task();
                    }
                }
            );
    }
}

template<class F, class... Args>
auto threadPool::enqueue(int priority, int order, F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

    std::future<return_type> res = task->get_future();

    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        if(stop)
            throw std::runtime_error("enqueue on stopped threadPool");
        
        tasks.emplace(Task(priority, order, std::function<void()>([task](){(*task)();})));
    }
    condition.notify_one();
    return res;
}

inline
threadPool::~threadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker: workers)
        worker.join();
}
#endif