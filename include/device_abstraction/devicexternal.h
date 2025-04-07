#ifndef DEVICEXTERNAL_H
#define DEVICEXTERNAL_H

#ifdef __cplusplus
#include <memory>
#include <map>
#include <atomic>
#include <mutex>
#endif

#include "dalcore.h"
#include "material/responseabstract.h"
#include "material/actionabstract.h"
#include "drivers/adapterinterface.h"
#include "asynch/actionqueue.h"
#include "asynch/queuemanager.h"

namespace dal { ////////////////////////////////////////////////////////////////

class LogicDevice;
class SimpleSynchInterface;
/**
 * @brief The DeviceInterface class
 */
class DeviceInterface
{
    friend LogicDevice;
    friend SimpleSynchInterface;

public:
    using interfaceHandle_t = std::shared_ptr<DeviceInterface>;

public:
    DeviceInterface();
    virtual ~DeviceInterface();

    virtual void notifyOwner(AbstractResponse::responseHandle_t);
    void execute    (AbstractAction::actionHandle_t);

protected:
    virtual void processAction(AbstractAction::actionHandle_t) = 0;

private:
    DAL_DECLARE_PIMPL

};
/**
 * @brief The LogicDevice class
 */
class LogicDevice
{
public:
    LogicDevice();
    virtual ~LogicDevice();

    virtual void responseGetter(std::unique_ptr<AbstractResponse>) = 0;

    void setInterface(DeviceInterface::interfaceHandle_t rhs);

    DeviceInterface::interfaceHandle_t
    interface() const;

protected:
    DAL_DECLARE_PIMPL

};
/**
 * @brief The SimpleSynchInterface class
 */
class SimpleSynchInterface : public DeviceInterface, public LogicDevice
{
public:
    SimpleSynchInterface    (std::unique_ptr<DeviceDriver>);
    ~SimpleSynchInterface   ();

    void processAction(AbstractAction::actionHandle_t) override;

    std::unique_ptr<AbstractResponse> getResponse();

private:
    using LogicDevice::setInterface;
    using LogicDevice::interface;

    void responseGetter(std::unique_ptr<AbstractResponse>) override;

private:
    DAL_DECLARE_PIMPL

};
/**
 * @brief The QueuedAsynchInterface class
 */
class QueuedAsynchInterface : public DeviceInterface
{
public:
    QueuedAsynchInterface(SimpleQueue::handle_t,
                          QueueManager::managerHandle_t);
    ~QueuedAsynchInterface();

    void notifyOwner(AbstractResponse::responseHandle_t resp) override;

    // NOTE this class using ONLY with priority actions
    void processAction(AbstractAction::actionHandle_t) override;

protected:
    virtual void startAsynchQueueProcessing(std::shared_ptr<QueueManager>) = 0;

    void removeInterfaceFromQueueBeforeDestruction();
    void stopFurtherResponseProcessing();

    QueueInterface& queue();

private:
    DAL_DECLARE_PIMPL

};
/**
 * @brief The InterfaceCallbackSynchronizationPromitive class
 */
class InterfaceCallbackSynchronizationPrimitive final
{
public:
    void acquire(AbstractAction::uid_t);
    void release(AbstractAction::uid_t);
    std::vector<AbstractAction::uid_t> release();
    bool released() { return m_releaseFlag.load(std::memory_order_acquire); }

private:
    std::mutex m_containerLocker;
    std::map<AbstractAction::uid_t, int> m_acquired;
    std::atomic_bool m_releaseFlag{ false };

};

} //////////////////////////////////////////////////////////////////////////////

#endif // DEVICEXTERNAL_H
