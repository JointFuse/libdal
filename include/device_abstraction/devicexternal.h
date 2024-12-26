#ifndef DEVICEXTERNAL_H
#define DEVICEXTERNAL_H

#ifdef __cplusplus
#include <memory>
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

    void notifyOwner(AbstractResponse::responseHandle_t);
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

    // NOTE this class using ONLY with priority actions
    void processAction(AbstractAction::actionHandle_t) override;

protected:
    virtual void startAsynchQueueProcessing(std::shared_ptr<QueueManager>) = 0;

private:
    DAL_DECLARE_PIMPL

};

} //////////////////////////////////////////////////////////////////////////////

#endif // DEVICEXTERNAL_H
