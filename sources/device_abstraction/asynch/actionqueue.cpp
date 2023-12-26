#include "actionqueue.h"

#include <iostream>
#include <stdexcept>

void ActionQueue::push_back(container_t::value_type act)
try {
    const std::lock_guard<decltype(m_queueSynch)> mutex_lock(m_queueSynch);
    m_queue.push_back(std::move(act));
}
catch(std::system_error& err) {
    std::cerr << err.what() << std::endl;
}

void ActionQueue::push_front(container_t::value_type act)
try {
    const std::lock_guard<decltype(m_queueSynch)> mutex_lock(m_queueSynch);
    m_queue.push_front(std::move(act));
}
catch(std::system_error& err) {
    std::cerr << err.what() << std::endl;
}

ActionQueue::container_t::value_type ActionQueue::pop_back()
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

ActionQueue::container_t::value_type ActionQueue::pop_front()
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

void ActionQueue::lockInterface(AbstractAction::uid_t uid)
{
    m_synch[uid].lock();
}

void ActionQueue::unlockInterface(AbstractAction::uid_t uid)
{
    m_synch[uid].unlock();
}

bool ActionQueue::tryLockInterface(AbstractAction::uid_t uid)
{
    return m_synch[uid].try_lock();
}

void ActionQueue::registerInterface(AbstractAction::uid_t uid)
{
    m_synch[uid];
}

void ActionQueue::removeInterface(AbstractAction::uid_t uid)
{
    if (hasInterface(uid))
        m_synch.erase(m_synch.find(uid));
}

bool ActionQueue::hasInterface(AbstractAction::uid_t uid)
{
    return m_synch.find(uid) != m_synch.end();
}
