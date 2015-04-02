#ifndef  COMMON_THIS_THREAD_H_
#define  COMMON_THIS_THREAD_H_

#include <pthread.h>
#include <stdint.h>
#include <syscall.h>
#include <time.h>
#include <unistd.h>

namespace comm {

class ThisThread {
public:
    /// Sleep in ms
    static void Sleep(int64_t time_ms) {
        if (time_ms > 0) {
            timespec ts = {time_ms / 1000, (time_ms % 1000) * 1000000 };
            nanosleep(&ts, &ts);
        }
    }
    /// Get thread id
    static int GetId() {
        return syscall(__NR_gettid);
    }
    /// Yield cpu
    static void Yield() {
        sched_yield();
    }
};

} // namespace comm

using comm::ThisThread;
#endif
