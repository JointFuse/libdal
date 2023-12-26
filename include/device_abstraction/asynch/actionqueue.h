#ifndef ACTIONQUEUE_H
#define ACTIONQUEUE_H

#include <deque>
#include <map>
#include <mutex>
#include <memory>

#include "../material/actionabstract.h"

class QueueInterface
{
public:
    /*
     * NOTE shared_ptr isn't thread-safe class
     * due to complicated contrucst, destruct
     * and modificate operations
     *
     * WARNING all specified operations must be
     * synchronized or performed in one thread
     * to avoid resource leakage
     *
     * NOTE C++20 specification introduced
     * partial template specialization of
     * std::atomic for std::shared_ptr<T>
     * that allows users to manipulate
     * shared_ptr objects atomically.
     * However, existing implementations are
     * typically not lock-free, rendering
     * std::atomic<std::shared_ptr> useless
     * for low-latency and real-time applications
     */
    using handle_t = std::shared_ptr<QueueInterface>;

public:
    virtual ~QueueInterface() = default;

    virtual void push_back  (AbstractAction::actionHandle_t) = 0;
    virtual void push_front (AbstractAction::actionHandle_t) = 0;

    virtual AbstractAction::actionHandle_t pop_back ()       = 0;
    virtual AbstractAction::actionHandle_t pop_front()       = 0;

    virtual int queueSize   () const                         = 0;

    virtual void lockInterface      (AbstractAction::uid_t)  = 0;
    virtual void unlockInterface    (AbstractAction::uid_t)  = 0;
    virtual bool tryLockInterface   (AbstractAction::uid_t)  = 0;
    virtual void registerInterface  (AbstractAction::uid_t)  = 0;
    virtual void removeInterface    (AbstractAction::uid_t)  = 0;
    virtual bool hasInterface       (AbstractAction::uid_t)  = 0;
};

class ActionQueue : public QueueInterface
{
public:
    using container_t = std::deque<AbstractAction::actionHandle_t>;

public:
    virtual ~ActionQueue() = default;

    void push_back  (typename container_t::value_type) override;
    void push_front (typename container_t::value_type) override;
    typename container_t::value_type pop_back    () override;
    typename container_t::value_type pop_front   () override;

    int queueSize() const override { return m_queue.size(); };

    void lockInterface      (AbstractAction::uid_t) override;
    void unlockInterface    (AbstractAction::uid_t) override;
    bool tryLockInterface   (AbstractAction::uid_t) override;
    void registerInterface  (AbstractAction::uid_t) override;
    void removeInterface    (AbstractAction::uid_t) override;
    bool hasInterface       (AbstractAction::uid_t) override;

private:
    container_t m_queue;
    std::map<AbstractAction::uid_t, std::mutex> m_synch;
    std::mutex m_queueSynch;

};

#endif // ACTIONQUEUE_H
