
#ifndef  COMMON_EVENT_H_
#define  COMMON_EVENT_H_

#include "mutex.h"

namespace comm {

class AutoResetEvent {
public:
    AutoResetEvent()
      : cv_(&mutex_), signaled_(false) {
    }
    /// Wait for signal
    void Wait() {
        MutexLock lock(&mutex_);
        while (!signaled_) {
            cv_.Wait();
        }
        signaled_ = false;
    }
    bool TimeWait(int64_t timeout) {
        MutexLock lock(&mutex_);
        if (!signaled_) {
            cv_.TimeWait(timeout);
        }
        bool ret = signaled_;
        signaled_ = false;
        return ret;
    }
    /// Signal one
    void Set() {
        MutexLock lock(&mutex_);
        signaled_ = true;
        cv_.Signal();
    }

private:
    Mutex mutex_;
    CondVar cv_;
    bool signaled_;
};

} // namespace comm

using comm::AutoResetEvent;

#endif  // COMMON_EVENT_H_
