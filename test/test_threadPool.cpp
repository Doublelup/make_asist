#include "../include/threadPool.h"
#include <iostream>

int func(int &i)
{
    std::cout << "hello " << i << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "world " << i << std::endl;
    return i*i;
}

int main()
{
    
    threadPool pool(4);
    std::vector< std::future<int> > results;

    for(int i = 0; i < 8; ++i) {
        results.emplace_back(
            pool.enqueue(0, func, i)
        );
    }

    for(auto && result: results)
        std::cout << result.get() << ' ';
    std::cout << std::endl;
    
    return 0;
}