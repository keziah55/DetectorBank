#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "thread_pool.h"

//#include <iostream>

ThreadPool::ThreadPool(std::size_t numThreads)
    : remain(0)
    , threads(numThreads ? numThreads : std::thread::hardware_concurrency())
{
    states = std::unique_ptr<enum state[]> {
        new enum state[threads]
    };
    exceptions = std::unique_ptr<std::exception_ptr[]> {
        new std::exception_ptr[threads]
    };
    for (std::size_t i {0} ; i < threads ; i++)
        states[i] = waiting;
    workers = std::unique_ptr<std::thread[]> {
        new std::thread[threads]
    };
    for (std::size_t i {0} ; i < threads ; i++) 
        workers[i] = std::thread(&ThreadPool::dispatcher, this, i);
}
    
ThreadPool::~ThreadPool()
{
    // Tell all the threads to die
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [this]{ return remain==0; });
    for (std::size_t i {0} ; i < threads ; i++)
        states[i] = dying;
    remain = threads;
    lk.unlock();
    cv.notify_all();
    for (std::size_t i {0} ; i < threads ; i++) {
        workers[i].join();
    }
}

void ThreadPool::wait_raise(std::unique_lock<std::mutex>& lk, const std::size_t to_check)
{
    cv.wait(lk, [this]{ return remain==0; });
    for (std::size_t i {0} ; i < to_check ; i++)
        if (exceptions[i])
            std::rethrow_exception(exceptions[i]);
}

void ThreadPool::manifold(delegate_t delegate,
                         void** params,
                         std::size_t jobs)
{
    this->params = params;
    this->delegate = delegate;
    remain = 0;
    std::size_t to_start {0};
    // Clear any old exceptions before reusing the threads
    for (std::size_t i {0} ; i < threads ; i++)
        exceptions[i] = nullptr;
    std::unique_lock<std::mutex> lk(m);    
    while(remain < jobs) {
        wait_raise(lk, to_start);
        this->params += to_start;
        jobs -= to_start;
        to_start = std::min(jobs, this->threads);
        for (std::size_t i {0} ; i < to_start ; i++)
            states[i] = i < to_start ? running : waiting;
        remain = to_start;
        lk.unlock();
        cv.notify_all();
        lk.lock();
    }
    wait_raise(lk, to_start);
}

void ThreadPool::dispatcher(int id)
{
    std::unique_lock<std::mutex> lk(m, std::defer_lock);
    while(true) {
        lk.lock();
        cv.wait(lk, [&id,this]{ return states[id] != waiting; });
        // Time to die?
        if (states[id] == dying) {
            --remain;
            states[id] = dead;
            lk.unlock();
            cv.notify_all();
            return;
        }
        lk.unlock();
        
        // Call delegate, passing parameter block
        try {
            delegate(params[id]);
        } catch(...) {
            exceptions[id] = std::current_exception();
        }
        
        lk.lock();
        states[id] = waiting;
        --remain;
        lk.unlock();
        cv.notify_all();
    }
}

