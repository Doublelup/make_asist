#include "../include/threadPool.h"

threadPool::threadPool(size_t maxnum)
:   stop{false}
{
    for(size_t i = 0; i < maxnum; ++i)
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

template<class F, class... Args>
auto threadPool::enqueue(int priority, F&& f, Args&&... args)
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
        
        tasks.emplace(Task(priority, std::function<void()>([task](){(*task)();})));
    }
    condition.notify_one();
    return res;
}

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