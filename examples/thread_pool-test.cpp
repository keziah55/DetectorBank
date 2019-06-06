/*
 * Test the thread_pool class
 * 
 * Compile with g++ -I.. ../thread_pool.cpp thread_pool-test.cpp -pthread
 */

#include <iostream>
#include <mutex>
#include <exception>
#include "thread_pool.h"

using namespace std;

constexpr size_t threads {0}; // concurrency 0=>platform optimum
constexpr size_t jobs {3};    // number of jobs to do

// Each delegate function will take a C-string and an int.
struct Args {
    const char *word;
    int num;
};

// We're going to fire off the thread pool four times
// with four different argument sets.
//
// Using void** avoids typecasting later.
// Alternatively, cast to void** in manifold() call.
void* words[][jobs] = {
    { new Args {"One", 1}, new Args {"Two", 2}, new Args {"Three", 3} },
    { new Args {"Four", 1}, new Args {"Five", 2}, new Args {"Six", 3} },
    { new Args {"One", 1}, new Args {"Two", -2}, new Args {"Three", 3} },
    { new Args {"Unus", 1}, new Args {"Duo", 2}, new Args {"Tres", 3} },
};

// The delegate funciton accepts a void*
// which it casts to point at its particular argument type.
// Alternatively, cast to ThreadPool::delegate_t in manifold() call.
void say(void* words) {
    Args* w(static_cast<Args*>(words));
    int how_many {w->num};
    
    if (how_many < 0)
        throw(invalid_argument("Iteration count is negative"));
    
    //static mutex m;
    //lock_guard<mutex> lk(m);

    for (int i = 0 ; i < how_many ; i++)
        cout << w->word << '\t';
    cout << endl;
}


int main()
{
    // Requesting 0 threads causes the number to be
    // equal to the number of CPU cores available on this platform
    ThreadPool p(threads);
    cout << p.threads << " worker threads are available on this platform.\n\n";
    
    // Direct invocation
    // minifold will return the number of jobs actually run,
    // so the following formula ensures maximum concurrency
    p.manifold(ThreadPool::delegate_t(say), words[0], jobs);
    cout << endl;
    
    // Invocation via lambda expression
    // permits use of, e.g., non-static member functions
    auto d { [](void* w){say(w);} };
    p.manifold(d, words[1], jobs);
    cout << endl;
    
    // Exception handling
    try {
        p.manifold(d, words[2], jobs);
    } catch(const std::invalid_argument& ia) {
        std::cerr << "Invalid_argument: " << ia.what() << std::endl;
    }
    cout << endl;
    
    // Foreign language support
    // (checks exceptions are cleared properly)
    p.manifold(d, words[3], jobs);
    cout << endl;
   
    cout << "\nFinished\n";
    // I should probably delete the Args in words[][] now...
    
    return 0;
}
