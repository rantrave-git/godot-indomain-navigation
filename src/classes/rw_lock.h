#pragma once

#ifdef MINGW_ENABLED
#define MINGW_STDTHREAD_REDUNDANCY_WARNING
#include "thirdparty/mingw-std-threads/mingw.shared_mutex.h"
#define THREADING_NAMESPACE mingw_stdthread
#else
#include <shared_mutex>
#define THREADING_NAMESPACE std
#endif

class RWLock {
public:    
    using lock_t=THREADING_NAMESPACE::shared_timed_mutex;
    using shared_lock_t=THREADING_NAMESPACE::shared_lock<lock_t>;
    // [TODO] try spin lock implementation
    // using spin_lock_t=SpinLock<lock_t>;
    // using shared_spin_lock_t=SpinLock<shared_lock_t>;

    using lock_guard_t=THREADING_NAMESPACE::lock_guard<lock_t>;
    using shared_lock_guard_t=THREADING_NAMESPACE::lock_guard<shared_lock_t>;
    // using spin_lock_guard_t=THREADING_NAMESPACE::lock_guard<spin_lock_t>;
    // using shared_spin_lock_guard_t=THREADING_NAMESPACE::lock_guard<shared_spin_lock_t>;

    RWLock() : 
        // shared_spin_lock{},
        shared_lock{mutex, THREADING_NAMESPACE::defer_lock},
        // spin_lock{},
        mutex{} {}
    // inline lock_t& get_lock() { return mutex; }
    // inline shared_lock_t& get_shared_lock() { return shared_lock; }
    // inline spin_lock_t& get_spin_lock() { return spin_lock; }
    // inline shared_spin_lock_t& get_shared_spin_lock() { return shared_spin_lock; }

    inline lock_guard_t write_lock() { return lock_guard_t{mutex}; }
    inline shared_lock_guard_t read_lock() const { return shared_lock_guard_t{shared_lock}; }
    // inline spin_lock_guard_t write_spin_lock() { return spin_lock_guard_t{spin_lock}; }
    // inline shared_spin_lock_guard_t read_spin_lock() const { return shared_spin_lock_guard_t{shared_spin_lock}; }
    
private:
    mutable lock_t mutex;
    mutable shared_lock_t shared_lock;
    // mutable spin_lock_t spin_lock;
    // mutable shared_spin_lock_t shared_spin_lock;
};
