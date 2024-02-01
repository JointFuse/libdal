#ifndef CHANNEL_H
#define CHANNEL_H

#include <mutex>
#include <atomic>
#include <type_traits>

namespace dal { ////////////////////////////////////////////////////////////////

using namespace std;
/**
 * @brief The MutexChannel class
 */
template<typename T>
class MutexChannel
{
public:
    MutexChannel() = default;
    MutexChannel(T data)
        : m_data{ data } { }

    void set(T newData) {
        auto lck = lock_guard<decltype(m_access)>{ m_access };
        m_data = newData;
    }
    T get() {
        auto lck = lock_guard<decltype(m_access)>{ m_access };
        return m_data;
    }

private:
    T m_data;
    recursive_mutex m_access;

};
/**
 * @brief The AtomicChannel class
 */
template<typename T>
class AtomicChannel
{
public:
    AtomicChannel() = default;
    AtomicChannel(T data) noexcept
        : m_data{ data } {}

    inline void set(T newData) noexcept {
        m_data = newData;
    }
    inline T get() noexcept {
        return m_data;
    }

private:
    atomic<T> m_data;

};
/**
 * @brief The Channel class
 */
template<typename T, typename = void>
class Channel {};
/**
 *
 */
template<typename T>
class Channel<T, enable_if_t<is_trivially_copyable_v<T> &&
                            is_copy_constructible_v<T> &&
                            is_move_constructible_v<T> &&
                            is_copy_assignable_v<T> &&
                            is_move_assignable_v<T>>>
    : public AtomicChannel<T>
{
    using AtomicChannel<T>::AtomicChannel;
};
/**
 *
 */
template<typename T>
class Channel<T, enable_if_t<!is_trivially_copyable_v<T> ||
                            !is_copy_constructible_v<T> ||
                            !is_move_constructible_v<T> ||
                            !is_copy_assignable_v<T> ||
                            !is_move_assignable_v<T>>>
    : public MutexChannel<T>
{
    using MutexChannel<T>::MutexChannel;
};

} //////////////////////////////////////////////////////////////////////////////

#endif // CHANNEL_H
