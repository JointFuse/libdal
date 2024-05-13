#include "actionqueue.h"

#include <deque>
#include <map>
#include <mutex>
#include <iostream>
#include <stdexcept>

using namespace dal;

class SimpleQueue::_impl
{
private:
    using container_t = std::deque<AbstractAction::actionHandle_t>;

public:
    void push_back(AbstractAction::actionHandle_t act)
    try {
        const std::lock_guard<decltype(m_queueSynch)> mutex_lock(m_queueSynch);
        m_queue.push_back(std::move(act));
    }
    catch(std::system_error& err) {
        std::cerr << err.what() << std::endl;
    }

    void push_front(AbstractAction::actionHandle_t act)
    try {
        const std::lock_guard<decltype(m_queueSynch)> mutex_lock(m_queueSynch);
        m_queue.push_front(std::move(act));
    }
    catch(std::system_error& err) {
        std::cerr << err.what() << std::endl;
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
        return {};
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
        return {};
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
        return {};
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
        return {};
    }

    void clear()
    try {
        const std::lock_guard<decltype(m_queueSynch)> mutex_lock(m_queueSynch);
        m_queue.clear();
    }
    catch(std::system_error& err) {
        std::cerr << err.what() << std::endl;
    }

    int queueSize() const
    {
        return m_queue.size();
    }

    void lockInterface(AbstractAction::uid_t uid)
    {
        const auto _ = std::lock_guard<decltype(m_synchMutex)>{ m_synchMutex };
        m_synch[uid].lock();
    }

    void unlockInterface(AbstractAction::uid_t uid)
    {
        const auto _ = std::lock_guard<decltype(m_synchMutex)>{ m_synchMutex };
        m_synch[uid].unlock();
    }

    bool tryLockInterface(AbstractAction::uid_t uid)
    {
        const auto _ = std::lock_guard<decltype(m_synchMutex)>{ m_synchMutex };
        return m_synch.find(uid) != m_synch.end() && m_synch[uid].try_lock();
    }

    void registerInterface(AbstractAction::uid_t uid)
    {
        const auto _ = std::lock_guard<decltype(m_synchMutex)>{ m_synchMutex };
        m_synch[uid];
    }

    void removeInterface(AbstractAction::uid_t uid)
    {
        const auto _ = std::lock_guard<decltype(m_synchMutex)>{ m_synchMutex };
        if (hasInterface(uid))
            m_synch.erase(m_synch.find(uid));
    }

    bool hasInterface(AbstractAction::uid_t uid)
    {
        const auto _ = std::lock_guard<decltype(m_synchMutex)>{ m_synchMutex };
        return m_synch.find(uid) != m_synch.end();
    }

private:
    container_t m_queue;
    std::map<AbstractAction::uid_t, std::mutex> m_synch;
    std::recursive_mutex m_synchMutex;
    std::mutex m_queueSynch;

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

void SimpleQueue::lockInterface(AbstractAction::uid_t uid)
{
    pimpl->lockInterface(uid);
}

void SimpleQueue::unlockInterface(AbstractAction::uid_t uid)
{
    pimpl->unlockInterface(uid);
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

DAL_PIMPL_DEFAULT_CONSTRUCTOR(SimpleQueue)
DAL_PIMPL_DEFAULT_DESTRUCTOR(SimpleQueue)
