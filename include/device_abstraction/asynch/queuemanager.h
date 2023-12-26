#ifndef QUEUEMANAGER_H
#define QUEUEMANAGER_H

#include <memory>
#include <atomic>
#include <future>
#include <thread>
#include <mutex>

#include "actionqueue.h"
#include "../drivers/adapterinterface.h"
/**
 * @brief The QueueManager class
 */
class QueueManager
{
public:
    using managerHandle_t = std::shared_ptr<QueueManager>;

public:
    QueueManager(std::unique_ptr<DeviceDriver> executor,
                 ActionQueue::handle_t queue);
    virtual ~QueueManager();

    virtual void processQueue() = 0;

    bool isWorking() const noexcept { return m_isWorking; }

protected:
    std::unique_ptr<DeviceDriver> m_executor;
    ActionQueue::handle_t m_queue;
    std::atomic<bool> m_isWorking;

};
/**
 * @brief The SimpleManager class
 */
class SimpleManager : public QueueManager
{
public:
    using QueueManager::QueueManager;

    void processQueue() override;

protected:
    virtual void sendClientResponse(AbstractResponse::responseHandle_t) = 0;

};
/**
 * @brief The AsynchRespondManager class
 */
class AsynchRespondManager : public SimpleManager
{
public:
    AsynchRespondManager(std::unique_ptr<DeviceDriver> executor,
                         ActionQueue::handle_t queue);
    ~AsynchRespondManager();

protected:
    void sendClientResponse(AbstractResponse::responseHandle_t) final;
    virtual void responseSender(AbstractResponse::responseHandle_t) = 0;

private:
    std::list<AbstractResponse::responseHandle_t> m_resp;
    std::future<void> m_task;
    std::atomic<bool> m_flag{ true };
    std::mutex m_mtx;

};

#endif // QUEUEMANAGER_H
