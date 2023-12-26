#include "devicexternal.h"

#include <stdexcept>
#include <iostream>
#include <future>

void LogicDevice::setInterface(DeviceInterface::interfaceHandle_t rhs)
{
    if (m_interface &&
        rhs.get() == m_interface.get())
        return;

    rhs->m_owner = this;
    m_interface = rhs;
}

void DeviceInterface::notifyOwner(AbstractResponse::responseHandle_t resp)
{
    if (m_owner == nullptr)
        return;

    m_owner->responseGetter(std::move(resp));
}

void DeviceInterface::execute(AbstractAction::actionHandle_t act)
{
    act->m_requestor = this;
    processAction(std::move(act));
}

SimpleSynchInterface::SimpleSynchInterface(std::unique_ptr<DeviceDriver> exc)
    : m_executor{ std::move(exc) }
{
    if (!m_executor)
        throw std::runtime_error{"synchronous interface initialized "
                                 "with NULL driver handle"};

    m_executor->initializeDevice();
}

SimpleSynchInterface::~SimpleSynchInterface()
{
    m_executor->closeDevice();
}

void SimpleSynchInterface::processAction(AbstractAction::actionHandle_t act)
try {
    auto res = m_executor->executeAction(std::move(act));
    notifyOwner(std::move(res));
}
catch(driver_error& err) {
    std::cerr << err.what() << std::endl;
}

QueuedAsynchInterface::QueuedAsynchInterface(ActionQueue::handle_t que,
                                             std::shared_ptr<QueueManager> mgr)
    : m_queue{ que }
    , m_manager{ mgr }
{
    if (!m_queue)
        throw std::runtime_error{"asynchronous interface initialized "
                                 "with NULL queue handle"};
    if (!m_manager)
        throw std::runtime_error{"asynchronous interface initialized "
                                 "with NULL manager handle"};

    m_queue->registerInterface(this);
}

QueuedAsynchInterface::~QueuedAsynchInterface()
{
    m_queue->lockInterface(this);
    m_queue->removeInterface(this);
}

void QueuedAsynchInterface::processAction(AbstractAction::actionHandle_t act)
{
    m_queue->push_back(std::move(act));

    if (!m_manager->isWorking())
        startAsynchQueueProcessing(m_manager);
}
