#ifndef FIBERCHANNELS_H
#define FIBERCHANNELS_H

#include <mutex>
#include <atomic>
#include <type_traits>
#include <condition_variable>
#include <vector>
#include <functional>
#include <set>
#include <chrono>

namespace dal { ////////////////////////////////////////////////////////////////

template<typename T>
class DecoChannel
{
public:
    DecoChannel() = default;
    DecoChannel(std::function<void(std::function<void()>)> setterDecorator)
        : m_setterDecorator{ setterDecorator }
    {}

    void set(T newData) {
        auto setter = [dt = std::forward(newData),
                       &mtx = m_access,
                       &internal = m_data]() {
            auto lck = std::lock_guard<decltype(mtx)>{ mtx };
            internal = dt;
        };

        if (m_setterDecorator)
            m_setterDecorator(setter);
        else
            setter();
    }
    T get() {
        auto lck = std::lock_guard<decltype(m_access)>{ m_access };
        return m_data;
    }

private:
    T m_data;
    std::recursive_mutex m_access;
    const std::function<void(std::function<void()>)> m_setterDecorator;

};

template<typename T>
class MultiChannel
{
    struct impl
    {
        std::mutex accMtx;
        std::condition_variable cv;
        std::set<int> ready;
    };

public:
    // NOTE support only creation of new channels, for removing need to
    // rework implementation of methods where numbers used as
    // identificators of concrete channel in vector container
    std::shared_ptr<DecoChannel<T>> getChannel();
    std::vector<T> any();
//    T all();

private:
    std::shared_ptr<impl> m_impl;
    std::vector<std::shared_ptr<DecoChannel<T>>> m_chnls;

};

template<typename T>
std::shared_ptr<DecoChannel<T>> MultiChannel<T>::getChannel()
{
    auto decor = [impl = m_impl,
                  curr = m_chnls.size()](std::function<void()> setter) {
        const auto _ = std::lock_guard<decltype(impl->accMtx)>(impl->accMtx);
        setter();
        impl->ready.insert(curr);
        impl->cv.notify_all();
    };

    m_chnls.push_back(std::make_shared<DecoChannel<T>>(decor));
    return m_chnls.back();
}

template<typename T>
std::vector<T> MultiChannel<T>::any()
{
    static constexpr auto TIMEOUT = std::chrono::milliseconds(3000);

    const auto lck = std::unique_lock<decltype(m_impl->accMtx)>(m_impl->accMtx);

    if (m_impl->ready.empty())
        m_impl->cv.wait(lck, TIMEOUT);

    auto res = std::vector<T>{};
    res.reserve(m_impl->ready.size());

    for (auto num : m_impl->ready)
        res.push_back(m_chnls[num].get());

    m_impl->ready.clear();
    return res;
}

} /// ~dal /////////////////////////////////////////////////////////////////////

#endif // FIBERCHANNELS_H
