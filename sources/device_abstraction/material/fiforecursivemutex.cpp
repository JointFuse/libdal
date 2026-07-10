#include "fiforecursivemutex.h"

void FifoRecursiveMutex::lock()
{
    // 1. Recursion
    if ( m_owner.load( std::memory_order_acquire ) == std::this_thread::get_id() ) {
        m_recursion_count.fetch_add( 1, std::memory_order_relaxed );
        return;
    }

    // 2. Get a ticket
    const std::uint64_t my_ticket = m_next_ticket.fetch_add( 1, std::memory_order_acq_rel );

    // 3. Waiting for one's turn
    while ( m_now_serving.load( std::memory_order_acquire ) != my_ticket ) {
        std::this_thread::yield();
    }

    // 4. Captured
    m_recursion_count.store( 1, std::memory_order_relaxed );
    m_owner.store( std::this_thread::get_id(), std::memory_order_release );
}

void FifoRecursiveMutex::unlock()
{
    // 1. Decrement the recursion counter
    const auto recursionBase = m_recursion_count.fetch_sub( 1, std::memory_order_relaxed );
    assert( recursionBase != 0 );
    if ( recursionBase > 1 ) {
        return;
    }

    // 2. Clearing out
    m_owner.store( std::thread::id{}, std::memory_order_release );

    // 3. Calling the next person with a ticket
    m_now_serving.fetch_add( 1, std::memory_order_acq_rel );
}

bool FifoRecursiveMutex::try_lock()
{
    if ( m_owner.load( std::memory_order_acquire ) == std::this_thread::get_id() ) {
        m_recursion_count.fetch_add( 1, std::memory_order_relaxed );
        return true;
    }

    // Trying to grab a ticket that is currently being processed
    std::uint64_t expected = m_now_serving.load( std::memory_order_acquire );
    if ( m_next_ticket.compare_exchange_strong(
             expected, expected + 1, std::memory_order_acq_rel ) ) {
        m_recursion_count.store( 1, std::memory_order_relaxed );
        m_owner.store( std::this_thread::get_id(), std::memory_order_release );
        return true;
    }
    return false;
}
