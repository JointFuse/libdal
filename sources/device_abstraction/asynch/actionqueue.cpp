#include "actionqueue.h"

#include <atomic>
#include <deque>
#include <map>
#include <mutex>
#include <iostream>
#include <stdexcept>
#include <shared_mutex>
#include <condition_variable>

using namespace dal;

class InterfaceAcquisitionSync
{
public:
    bool acquire() {
        if (!isAlive())
            return false;

        ++m_acquireCounter;
        return true;
    }

    void release() {
        if (m_acquireCounter == 0)
            throw std::runtime_error("attempt to unlock non-locked interface");

        m_acquireCounter -= 1;

        if (m_acquireCounter != 0)
            return;

        m_releaseNotifier.notify_all();
    }

    template<typename Mutex>
    void waitForRelease(std::unique_lock<Mutex>& mtx) {
        if (m_acquireCounter == 0)
            return;

        m_releaseNotifier.wait
            (mtx, [&flag = m_acquireCounter] () { return flag == 0; });
    }

    bool isAlive() const {
        return m_aliveFlag.load(std::memory_order_acquire);
    }

    void kill() {
        m_aliveFlag.store(false, std::memory_order_release);
    }

private:
    std::atomic_int m_acquireCounter{ 0 };
    std::condition_variable_any m_releaseNotifier;
    std::atomic_bool m_aliveFlag{ true };

};

class SimpleQueue::_impl
{
public:
    using container_t = std::deque<AbstractAction::actionHandle_t>;

    friend FroQueue;

public:
    void push_back(AbstractAction::actionHandle_t act)
    try {
        const std::lock_guard<decltype(m_queueSynch)> mutex_lock(m_queueSynch);
        m_queue.push_back(std::move(act));
    }
    catch(std::system_error& err) {
        std::cerr << err.what() << std::endl;
        throw err;
    }

    void push_front(AbstractAction::actionHandle_t act)
    try {
        const std::lock_guard<decltype(m_queueSynch)> mutex_lock(m_queueSynch);
        m_queue.push_front(std::move(act));
    }
    catch(std::system_error& err) {
        std::cerr << err.what() << std::endl;
        throw err;
    }

    AbstractAction::actionHandle_t pop_back()
    try {
        const std::lock_guard<decltype(m_queueSynch)> mutex_lock(m_queueSynch);
        auto buf{ std::move(m_queue.back()) };
        m_queue.pop_back();
        return buf;
    }
    catch(std::system_error& err) {
        std::cerr << err.what() << std::endl;
        throw err;
    }

    AbstractAction::actionHandle_t pop_front()
    try {
        const std::lock_guard<decltype(m_queueSynch)> mutex_lock(m_queueSynch);
        auto buf{ std::move(m_queue.front()) };
        m_queue.pop_front();
        return buf;
    }
    catch(std::system_error& err) {
        std::cerr << err.what() << std::endl;
        throw err;
    }

    bool pop_back(AbstractAction::actionHandle_t& hndl)
    try {
        const std::lock_guard<decltype(m_queueSynch)> mutex_lock(m_queueSynch);

        if (m_queue.empty())
            return false;

        hndl.swap(m_queue.back());
        m_queue.pop_back();
        return true;
    }
    catch(std::system_error& err) {
        std::cerr << err.what() << std::endl;
        throw err;
    }

    bool pop_front(AbstractAction::actionHandle_t& hndl)
    try {
        const std::lock_guard<decltype(m_queueSynch)> mutex_lock(m_queueSynch);

        if (m_queue.empty())
            return false;

        hndl.swap(m_queue.front());
        m_queue.pop_front();
        return true;
    }
    catch(std::system_error& err) {
        std::cerr << err.what() << std::endl;
        throw err;
    }

    void clear()
    try {
        const std::lock_guard<decltype(m_queueSynch)> mutex_lock(m_queueSynch);
        m_queue.clear();
    }
    catch(std::system_error& err) {
        std::cerr << err.what() << std::endl;
        throw err;
    }

    int queueSize() const {
        return m_queue.size();
    }

    void lockInterface(AbstractAction::uid_t uid) = delete;

    bool isAlive(AbstractAction::uid_t uid) {
        const auto _ = std::shared_lock<decltype(m_synchMutex)>{ m_synchMutex };
        return m_synch.find(uid) != m_synch.end() && m_synch.at(uid).isAlive();
    }

    bool checkAliveAndUnlockInterface(AbstractAction::uid_t uid) {
        const auto _ = std::shared_lock<decltype(m_synchMutex)>{ m_synchMutex };
        const auto isInterfaceAlive = m_synch.at(uid).isAlive();
        m_synch.at(uid).release();
        return isInterfaceAlive;
    }

    bool tryLockInterface(AbstractAction::uid_t uid) {
        const auto _ = std::shared_lock<decltype(m_synchMutex)>{ m_synchMutex };

        if (m_synch.find(uid) != m_synch.end() && m_synch[uid].acquire())
            return true;
        else
            return false;
    }

    void registerInterface(AbstractAction::uid_t uid) {
        const auto _ = std::lock_guard<decltype(m_synchMutex)>{ m_synchMutex };
        m_synch[uid];
    }

    void removeInterface(AbstractAction::uid_t uid) {
        if (!hasInterface(uid))
            return;

        auto _ = std::unique_lock<decltype(m_synchMutex)>{ m_synchMutex };
        m_synch[uid].waitForRelease(_);
        m_synch.erase(m_synch.find(uid));
    }

    bool hasInterface(AbstractAction::uid_t uid) {
        const auto _ = std::shared_lock<decltype(m_synchMutex)>{ m_synchMutex };
        return m_synch.find(uid) != m_synch.end();
    }

    void killInterface(AbstractAction::uid_t uid) {
        auto _ = std::shared_lock<decltype(m_synchMutex)>{ m_synchMutex };
        m_synch.at(uid).kill();
    }

private:
    container_t m_queue;
    std::map<AbstractAction::uid_t, InterfaceAcquisitionSync> m_synch;
    std::shared_mutex m_synchMutex;
    std::mutex m_queueSynch;

};

class FroQueue::_impl
{
public:
    _impl(SimpleQueue* base,
          SimpleQueue::_impl::container_t& cntr,
          std::mutex& queueSynch)
        : m_base{ base }
        , m_queue{ cntr }
        , m_queueSynch{ queueSynch } {
        if (m_base == nullptr)
            throw std::runtime_error("dal queue initialization error");
    }

    void push_back(AbstractAction::actionHandle_t act)
    {
        const auto readyAct = dynamic_cast<ReadyAction*>(act.get());

        if (readyAct == nullptr)
            throw std::runtime_error("dal queue used with wrong action subtype");

        m_base->SimpleQueue::push_back(std::move(act));
    }

    void push_front(AbstractAction::actionHandle_t act)
    {
        const auto readyAct = dynamic_cast<ReadyAction*>(act.get());

        if (readyAct == nullptr)
            throw std::runtime_error("dal queue used with wrong action subtype");

        m_base->SimpleQueue::push_front(std::move(act));
    }

    AbstractAction::actionHandle_t pop_back()
    try {
        const std::lock_guard<decltype(m_queueSynch)> mutex_lock(m_queueSynch);
        auto buf = AbstractAction::actionHandle_t{};

        for (auto itr = m_queue.begin(); itr != m_queue.end(); ++itr)
        {
            const auto readyAct = dynamic_cast<ReadyAction*>(itr->get());

            if (!readyAct || !readyAct->isReady())
                continue;

            buf.swap(*itr);
            m_queue.erase(itr);
            break;
        }

        return buf;
    }
    catch(std::system_error& err) {
        std::cerr << err.what() << std::endl;
        throw err;
    }

    AbstractAction::actionHandle_t pop_front()
    try {
        const std::lock_guard<decltype(m_queueSynch)> mutex_lock(m_queueSynch);
        auto buf = AbstractAction::actionHandle_t{};

        for (auto itr = m_queue.rbegin(); itr != m_queue.rend(); ++itr)
        {
            const auto readyAct = dynamic_cast<ReadyAction*>(itr->get());

            if (!readyAct || !readyAct->isReady())
                continue;

            buf.swap(*itr);
            m_queue.erase(itr.base());
            break;
        }

        return buf;
    }
    catch(std::system_error& err) {
        std::cerr << err.what() << std::endl;
        throw err;
    }

    bool pop_back(AbstractAction::actionHandle_t& hndl)
    try {
        const std::lock_guard<decltype(m_queueSynch)> mutex_lock(m_queueSynch);

        if (m_queue.empty())
            return false;

        for (auto itr = m_queue.begin(); itr != m_queue.end(); ++itr)
        {
            const auto readyAct = dynamic_cast<ReadyAction*>(itr->get());

            if (!readyAct)
                return false;

            if (!readyAct->isReady())
                continue;

            hndl.swap(*itr);
            m_queue.erase(itr);
            break;
        }

        return true;
    }
    catch(std::system_error& err) {
        std::cerr << err.what() << std::endl;
        throw err;
    }

    bool pop_front(AbstractAction::actionHandle_t& hndl)
    try {
        const std::lock_guard<decltype(m_queueSynch)> mutex_lock(m_queueSynch);

        if (m_queue.empty())
            return false;

        for (auto i = (int)m_queue.size() - 1; 0 <= i; --i)
        {
            const auto readyAct = dynamic_cast<ReadyAction*>(m_queue[i].get());

            if (!readyAct)
                return false;

            if (!readyAct->isReady())
                continue;

            hndl.swap(m_queue.at(i));
            m_queue.erase(m_queue.begin() + i);
            break;
        }

        return true;
    }
    catch(std::system_error& err) {
        std::cerr << err.what() << std::endl;
        throw err;
    }

private:
    SimpleQueue* m_base;
    SimpleQueue::_impl::container_t& m_queue;
    std::mutex& m_queueSynch;

};

void SimpleQueue::push_back(AbstractAction::actionHandle_t act)
{
    pimpl->push_back(std::move(act));
}

void SimpleQueue::push_front(AbstractAction::actionHandle_t act)
{
    pimpl->push_front(std::move(act));
}

AbstractAction::actionHandle_t SimpleQueue::pop_back()
{
    return pimpl->pop_back();
}

AbstractAction::actionHandle_t SimpleQueue::pop_front()
{
    return pimpl->pop_front();
}

bool SimpleQueue::pop_back(AbstractAction::actionHandle_t& hndl)
{
    return pimpl->pop_back(hndl);
}

bool SimpleQueue::pop_front(AbstractAction::actionHandle_t& hndl)
{

    return pimpl->pop_front(hndl);
}

void SimpleQueue::clear()
{
    pimpl->clear();
}

int SimpleQueue::queueSize() const
{
    return pimpl->queueSize();
}

//void SimpleQueue::lockInterface(AbstractAction::uid_t uid)
//{
//    pimpl->lockInterface(uid);
//}

bool SimpleQueue::isAlive(AbstractAction::uid_t uid)
{
    return pimpl->isAlive(uid);
}

bool SimpleQueue::checkAliveAndUnlockInterface(AbstractAction::uid_t uid)
{
    return pimpl->checkAliveAndUnlockInterface(uid);
}

bool SimpleQueue::tryLockInterface(AbstractAction::uid_t uid)
{
    return pimpl->tryLockInterface(uid);
}

void SimpleQueue::registerInterface(AbstractAction::uid_t uid)
{
    pimpl->registerInterface(uid);
}

void SimpleQueue::removeInterface(AbstractAction::uid_t uid)
{
    pimpl->removeInterface(uid);
}

bool SimpleQueue::hasInterface(AbstractAction::uid_t uid)
{
    return pimpl->hasInterface(uid);
}

void SimpleQueue::killInterface(AbstractAction::uid_t uid)
{
    pimpl->killInterface(uid);
}

DAL_PIMPL_DEFAULT_CONSTRUCTOR(SimpleQueue)
DAL_PIMPL_DEFAULT_DESTRUCTOR(SimpleQueue)

FroQueue::FroQueue()
    : pimpl{ std::make_unique<_impl>(this,
                                    SimpleQueue::pimpl->m_queue,
                                    SimpleQueue::pimpl->m_queueSynch) }
{

}

void FroQueue::push_back(AbstractAction::actionHandle_t act)
{
    pimpl->push_back(std::move(act));
}

void FroQueue::push_front(AbstractAction::actionHandle_t act)
{
    pimpl->push_front(std::move(act));
}

AbstractAction::actionHandle_t FroQueue::pop_back()
{
    return pimpl->pop_back();
}

AbstractAction::actionHandle_t FroQueue::pop_front()
{
    return pimpl->pop_front();
}

bool FroQueue::pop_back(AbstractAction::actionHandle_t& act)
{
    return pimpl->pop_back(act);
}

bool FroQueue::pop_front(AbstractAction::actionHandle_t& act)
{
    return pimpl->pop_front(act);
}

DAL_PIMPL_DEFAULT_DESTRUCTOR(FroQueue)
