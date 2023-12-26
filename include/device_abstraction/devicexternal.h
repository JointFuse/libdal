#ifndef DEVICEXTERNAL_H
#define DEVICEXTERNAL_H

#include <memory>

#include "material/responseabstract.h"
#include "material/actionabstract.h"
#include "drivers/adapterinterface.h"
#include "asynch/actionqueue.h"
#include "asynch/queuemanager.h"

class LogicDevice;
/**
 * @brief The DeviceInterface class
 */
class DeviceInterface
{
    friend LogicDevice;

public:
    using interfaceHandle_t = std::shared_ptr<DeviceInterface>;

public:
    virtual ~DeviceInterface() = default;

    void notifyOwner(AbstractResponse::responseHandle_t);
    void execute    (AbstractAction::actionHandle_t);

protected:
    virtual void processAction(AbstractAction::actionHandle_t) = 0;

private:
    LogicDevice* m_owner;

};
/**
 * @brief The LogicDevice class
 */
class LogicDevice
{
public:
    virtual ~LogicDevice() = default;

    virtual void responseGetter(std::unique_ptr<AbstractResponse>) = 0;

    void setInterface           (DeviceInterface::interfaceHandle_t rhs);
    const DeviceInterface::interfaceHandle_t interface () const noexcept { return m_interface; }

protected:
    DeviceInterface::interfaceHandle_t m_interface;

};
/**
 * @brief The SimpleSynchInterface class
 */
class SimpleSynchInterface : public DeviceInterface
{
public:
    SimpleSynchInterface    (std::unique_ptr<DeviceDriver>);
    ~SimpleSynchInterface   ();

    void processAction(AbstractAction::actionHandle_t) override;

private:
    std::unique_ptr<DeviceDriver> m_executor;

};
/**
 * @brief The QueuedAsynchInterface class
 */
class QueuedAsynchInterface : public DeviceInterface
{
public:
    QueuedAsynchInterface(ActionQueue::handle_t,
                          QueueManager::managerHandle_t);
    ~QueuedAsynchInterface();

    void processAction(AbstractAction::actionHandle_t) override;

protected:
    virtual void startAsynchQueueProcessing(std::shared_ptr<QueueManager>) = 0;

private:
    ActionQueue::handle_t m_queue;
    std::shared_ptr<QueueManager> m_manager;

};

#endif // DEVICEXTERNAL_H
