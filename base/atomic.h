#ifndef _COMM_ATOMIC_H_
#define _COMM_ATOMIC_H_

#if !defined(__i386__) && !defined(__x86_64__)
#error    "Arch not supprot!"
#endif

#include <stdint.h>

namespace comm {

template <typename T>
inline void atomic_inc(volatile T* n)
{
    asm volatile ("lock; incl %0;":"+m"(*n)::"cc");
}
template <typename T>
inline void atomic_dec(volatile T* n)
{
    asm volatile ("lock; decl %0;":"+m"(*n)::"cc");
}
template <typename T>
inline T atomic_add_ret_old(volatile T* n, T v)
{
    asm volatile ("lock; xaddl %1, %0;":"+m"(*n),"+r"(v)::"cc");
    return v;
}
template <typename T>
inline T atomic_inc_ret_old(volatile T* n)
{
    T r = 1;
    asm volatile ("lock; xaddl %1, %0;":"+m"(*n), "+r"(r)::"cc");
    return r;
}
template <typename T>
inline T atomic_dec_ret_old(volatile T* n)
{
    T r = (T)-1;
    asm volatile ("lock; xaddl %1, %0;":"+m"(*n), "+r"(r)::"cc");
    return r;
}
template <typename T>
inline T atomic_add_ret_old64(volatile T* n, T v)
{
    asm volatile ("lock; xaddq %1, %0;":"+m"(*n),"+r"(v)::"cc");
    return v;
}
template <typename T>
inline T atomic_inc_ret_old64(volatile T* n)
{
    T r = 1;
    asm volatile ("lock; xaddq %1, %0;":"+m"(*n), "+r"(r)::"cc");
    return r;
}
template <typename T>
inline T atomic_dec_ret_old64(volatile T* n)
{
    T r = (T)-1;
    asm volatile ("lock; xaddq %1, %0;":"+m"(*n), "+r"(r)::"cc");
    return r;
}
template <typename T>
inline void atomic_add(volatile T* n, T v)
{
    asm volatile ("lock; addl %1, %0;":"+m"(*n):"r"(v):"cc");
}
template <typename T>
inline void atomic_sub(volatile T* n, T v)
{
    asm volatile ("lock; subl %1, %0;":"+m"(*n):"r"(v):"cc");
}
template <typename T, typename C, typename D>
inline T atomic_cmpxchg(volatile T* n, C cmp, D dest)
{
    asm volatile ("lock; cmpxchgl %1, %0":"+m"(*n), "+r"(dest), "+a"(cmp)::"cc");
    return cmp;
}
// return old value
template <typename T>
inline T atomic_swap(volatile T* lockword, T value)
{
    asm volatile ("lock; xchg %0, %1;" : "+r"(value), "+m"(*lockword));
    return value;
}
template <typename T, typename E, typename C>
inline T atomic_comp_swap(volatile T* lockword, E exchange, C comperand)
{
    return atomic_cmpxchg(lockword, comperand, exchange);
}

class BasicCounter
{
public:
    BasicCounter() : _counter(0) {}
    BasicCounter(uint32_t init) : _counter(init) {}
    uint32_t operator ++ ()
    {
        return ++ _counter;
    }
    uint32_t operator -- ()
    {
        return -- _counter;
    }
    operator uint32_t () const
    {
        return _counter;
    }
private:
    uint32_t _counter;
};

class AtomicCounter
{
public:
    AtomicCounter() : _counter(0) {}
    AtomicCounter(uint32_t init) : _counter(init) {}
    uint32_t operator ++ ()
    {
        return atomic_inc_ret_old(&_counter) + 1U;
    }
    uint32_t operator -- ()
    {
        return atomic_dec_ret_old(&_counter) - 1U;
    }
    operator uint32_t () const
    {
        return _counter;
    }
private:
    volatile uint32_t _counter;
};

class AtomicCounter64
{
public:
    AtomicCounter64() : _counter(0) {}
    AtomicCounter64(uint64_t init) : _counter(init) {}
    uint64_t operator ++ ()
    {
        return atomic_inc_ret_old64(&_counter) + 1LU;
    }
    uint64_t operator -- ()
    {
        return atomic_dec_ret_old64(&_counter) - 1LU;
    }
    operator uint64_t () const
    {
        return _counter;
    }
private:
    volatile uint64_t _counter;
};

} // namespace comm

#endif

/* vim: set ts=4 sw=4 sts=4 tw=100 */
