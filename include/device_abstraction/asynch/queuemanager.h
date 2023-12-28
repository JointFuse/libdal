#ifndef QUEUEMANAGER_H
#define QUEUEMANAGER_H

#include <memory>

#include "../dalcore.h"
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
                 SimpleQueue::handle_t queue);
    virtual ~QueueManager();

    virtual void processQueue() = 0;

    bool isWorking() const;

protected:
    DAL_DECLARE_PIMPL

};
/**
 * @brief The SimpleManager class
 */
class SimpleManager : public QueueManager
{
public:
    SimpleManager(std::unique_ptr<DeviceDriver> executor,
                  SimpleQueue::handle_t queue);
    ~SimpleManager();

    void processQueue() override;

protected:
    virtual void sendClientResponse(AbstractResponse::responseHandle_t) = 0;

private:
    DAL_DECLARE_PIMPL

};
/**
 * @brief The AsynchRespondManager class
 */
class AsynchRespondManager : public SimpleManager
{
public:
    AsynchRespondManager(std::unique_ptr<DeviceDriver> executor,
                         SimpleQueue::handle_t queue);
    ~AsynchRespondManager();

protected:
    void sendClientResponse(AbstractResponse::responseHandle_t) final;
    virtual void responseSender(AbstractResponse::responseHandle_t) = 0;

private:
    DAL_DECLARE_PIMPL

};

#endif // QUEUEMANAGER_H
