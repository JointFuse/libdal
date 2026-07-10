#ifndef FIFORECURSIVEMUTEX_H
#define FIFORECURSIVEMUTEX_H

#include <atomic>
#include <cassert>
#include <thread>

class FifoRecursiveMutex
{
  public:
    FifoRecursiveMutex() = default;

    FifoRecursiveMutex( const FifoRecursiveMutex& )            = delete;
    FifoRecursiveMutex& operator=( const FifoRecursiveMutex& ) = delete;

    void lock();
    void unlock();
    bool try_lock();

  private:
    // Two counters for Ticket Lock (guaranteeing strict FIFO)
    std::atomic<std::uint64_t> m_next_ticket{ 0 };
    std::atomic<std::uint64_t> m_now_serving{ 0 };

    std::atomic<std::thread::id> m_owner{ std::thread::id{} };
    std::atomic<std::uint32_t> m_recursion_count{ 0 };
};

#endif    // FIFORECURSIVEMUTEX_H
