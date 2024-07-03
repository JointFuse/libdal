#ifndef FIBERCHANNELS_H
#define FIBERCHANNELS_H

#include <mutex>
#include <atomic>
#include <type_traits>
#include <condition_variable>
#include <functional>
#include <map>
#include <set>
#include <chrono>

namespace dal { ////////////////////////////////////////////////////////////////

template<typename T>
class DecoChannel
{
private:
    template<typename Key, typename TT>
    friend class MultiChannel;

public:
    DecoChannel()
        : m_dataMutex{ std::make_shared<std::recursive_mutex>() }
    {}
    DecoChannel(std::function<void(std::function<void()>)> setterDecorator,
                std::shared_ptr<std::recursive_mutex> mutex)
        : m_dataMutex{ mutex }
        , m_setterDecorator{ setterDecorator }
    {}

    void set(T newData) {
        auto setter = [dt = newData,
                       &internal = m_data]() {
            internal = dt;
        };

        if (m_setterDecorator)
            m_setterDecorator(setter);
        else
        {
            auto lck = std::lock_guard<std::recursive_mutex>{ *m_dataMutex };
            m_data = newData;
        }
    }
    T get() {
        const auto lck = std::lock_guard<std::recursive_mutex>{ *m_dataMutex };
        return m_data;
    }

private:
    T get(std::shared_ptr<std::recursive_mutex> mtx) {
        if (mtx && mtx.owner_before(m_dataMutex)) {
            return m_data;
        }
        else
            return get();
    }

private:
    std::atomic_int m_getterCounter{ 0 };
    T m_data;
    std::shared_ptr<std::recursive_mutex> m_dataMutex;
    const std::function<void(std::function<void()>)> m_setterDecorator;

};

template<typename Key, typename T>
class MultiChannel
{
    struct impl
    {
        std::shared_ptr<std::recursive_mutex> accMtx{ std::make_shared<std::recursive_mutex>() };
        std::condition_variable_any cv;
        std::map<void*, std::set<Key>> ready;
    };

public:
    std::shared_ptr<DecoChannel<T>> getChannel(Key k);
    std::map<Key, T> any(void* reqId);
//    std::map<Key, T> all();

private:
    std::shared_ptr<impl> m_impl{ std::make_shared<impl>() };
    std::map<Key, std::shared_ptr<DecoChannel<T>>> m_chnls;

};

template<typename Key, typename T>
std::shared_ptr<DecoChannel<T>> MultiChannel<Key, T>::getChannel(Key k)
{
    auto decor = [impl = m_impl,
                  curr = k](std::function<void()> setter) {
        const auto _ = std::lock_guard<std::recursive_mutex>(*impl->accMtx);
        setter();

        for (auto& obj : impl->ready)
            obj.second.insert(curr);

        impl->cv.notify_all();
    };

    m_chnls[k] = std::make_shared<DecoChannel<T>>(decor, m_impl->accMtx);
    return m_chnls[k];
}

template<typename Key, typename T>
std::map<Key, T> MultiChannel<Key, T>::any(void* reqId)
{
    static constexpr auto TIMEOUT = std::chrono::milliseconds(3000);

    auto lck = std::unique_lock<std::recursive_mutex>(*m_impl->accMtx);

    if (m_impl->ready[reqId].empty())
    {
        const auto res = m_impl->cv.wait_for(lck, TIMEOUT);

        if (res == std::cv_status::timeout)
            return {};
    }

    auto res = std::map<Key, T>{};

    for (auto key : m_impl->ready[reqId])
        res[key] = m_chnls[key]->get(m_impl->accMtx);

    m_impl->ready[reqId].clear();
    return res;
}

} /// ~dal /////////////////////////////////////////////////////////////////////

#endif // FIBERCHANNELS_H
