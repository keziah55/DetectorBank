#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <stdexcept>

/*!
 * A utility class to permit a single function to be run on multiple
 * datasets concurrently.
 * 
 * The class may be instanced giving the number of concurrent threads
 * to run, or by requesting 0 threads (the default value), a number
 * will be chosen equal to the inherent concurrency of the platform
 * on which the application will run.
 * 
 * The caller must prepare an array of parameter blocks ("jobs"). These
 * along with the delegate function are passed to the manifold()
 * method, and each concurrent invocation of the delegate function
 * will be passed a separate parameter block.
 * 
 * The number of jobs must be defined in the manifold() call
 * but need not be equal to the number of threads. If the number
 * of jobs exceeds the number of threads, work will be performed
 * in batches. The size of each batch is the number of threads
 * given when the ThreadPool object is created. manifold()
 * does not return until all jobs are complete.
 * 
 * Threads are reused across calls to manifold(). When the ThreadPool
 * is destroyed, it closes down all threads and awaits their
 * proper termination.
 */
class ThreadPool {
public:
    /*! The type of the delegate function each thread will run */
    typedef std::function<void(void*)> delegate_t;

protected:
    /*! thread state descriptor */
    enum state {waiting, running, dying, dead};
    
    /*! Worker threads*/
    std::unique_ptr<std::thread[]> workers;

    /*! State variables for each thread */
    std::unique_ptr<enum state[]> states;
    
    /*! exception_ptrs for each thread */
    std::unique_ptr<std::exception_ptr[]> exceptions;
    
    /*! Parameters for each thread, passed to the runJobs function */
    void** params;
    
    /*! The work the workers are supposed to perform */
    delegate_t delegate;
    
    /*! Mutex to control access to job packets */
    std::mutex m;
    /*!
     * Condition variable through which to notify workers
     * that jobs are available
     */
    std::condition_variable cv;
    /*!
     * Count of the number of workers still to complete
     * their job packets
     */
    std::size_t remain;
    /*!
     * Convenience function: waits for a lock then deals with
     * any exceptions propagated from the threads
     * 
     * \param lk       The lock on which to wait
     * \param to_check Number of threads running on this lock
     */
    void wait_raise(std::unique_lock<std::mutex>& lk, const std::size_t to_check);

    /*!
     * Dispatch jobs to the given delegate
     * \param id index into the params/states arrays
     *           to be used by this thread
     */ 
    void dispatcher(int id);
    
public:
    /*! Number of threads in pool */
    const std::size_t threads;
    /*! Construct a thread pool */
    ThreadPool(std::size_t numThreads = 0);
    /*!
     * Destroy the thread pool.
     * Waits for all executive threads to terminate.
     */
    ~ThreadPool();
    /*!
     * Execute a function in each of the threads,
     * passing each one its own paramater set.
     * The number of jobs (== the number of paramater sets)
     * is passed in the final argument. If this exceeds
     * the number of threads in the pool, the jobs will be
     * sequenced into batches. mainfold() will only return
     * when all have been completed.
     * \param delegate The function each thread should call.
     * \param params   An array of pointers to parameters to pass.
     * \param jobs     Number of threads to run.
     */
    void manifold(delegate_t delegate,
                  void** params,
                  std::size_t jobs);
};

#endif
