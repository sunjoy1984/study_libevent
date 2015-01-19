#ifndef  __BLOCK_QUEUE_H_
#define  __BLOCK_QUEUE_H_
#include <deque>
#include <stdint.h>
#include "base/mutex.h"
#include "base/condition.h"

namespace comm{
template <class T>
class BlockQueue {
    public:
        explicit BlockQueue() :
            _mutex(),
            _cond(_mutex){
        }

        T take() {
            SafeMutex lock(_mutex);
            while(0 == _queue.size()){
                _cond.Wait();
            }

            T ret = _queue.front();
            _queue.pop_front();
            return ret;
        }

        void put(const T& data) {
            SafeMutex lock(_mutex);
            _queue.push_back(data);
            _cond.Notify();
        }

    private:
        Mutex _mutex;
        Condition _cond;
        std::deque<T> _queue;
};
}
#endif  //__BLOCK_QUEUE_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
