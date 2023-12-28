#ifndef ACTIONQUEUE_H
#define ACTIONQUEUE_H

#include <memory>

#include "../dalcore.h"
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

class SimpleQueue : public QueueInterface
{
public:
    SimpleQueue();
    ~SimpleQueue();

    void push_back  (AbstractAction::actionHandle_t) override;
    void push_front (AbstractAction::actionHandle_t) override;
    AbstractAction::actionHandle_t pop_back () override;
    AbstractAction::actionHandle_t pop_front() override;

    int queueSize() const override;

    void lockInterface      (AbstractAction::uid_t) override;
    void unlockInterface    (AbstractAction::uid_t) override;
    bool tryLockInterface   (AbstractAction::uid_t) override;
    void registerInterface  (AbstractAction::uid_t) override;
    void removeInterface    (AbstractAction::uid_t) override;
    bool hasInterface       (AbstractAction::uid_t) override;

private:
    DAL_DECLARE_PIMPL

};

#endif // ACTIONQUEUE_H
