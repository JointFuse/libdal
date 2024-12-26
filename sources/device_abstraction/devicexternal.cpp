#include "devicexternal.h"

#include <stdexcept>
#include <iostream>
#include <future>

using namespace dal;

class DeviceInterface::_impl
{
public:
    _impl(DeviceInterface* base)
        : m_base{ base } {

    }

    void notifyOwner(AbstractResponse::responseHandle_t resp)
    {
        if (m_owner == nullptr)
            return;

        m_owner->responseGetter(std::move(resp));
    }

    void execute(AbstractAction::actionHandle_t act)
    {
        act->m_requestor = m_base;
        m_base->processAction(std::move(act));
    }

public:
    LogicDevice* m_owner{ nullptr };

private:
    DeviceInterface* m_base;

};

class LogicDevice::_impl
{
public:
    _impl(LogicDevice* base)
        : m_base{ base } {

    }

    void setInterface(DeviceInterface::interfaceHandle_t rhs)
    {
        if (m_interface &&
            rhs.get() == m_interface.get())
            return;

        rhs->pimpl->m_owner = m_base;
        m_interface = rhs;
    }

    const DeviceInterface::interfaceHandle_t
    interface () const noexcept {
        return m_interface;
    }

private:
    DeviceInterface::interfaceHandle_t m_interface;
    LogicDevice* m_base;

};

class SimpleSynchInterface::_impl
{
public:
    _impl(SimpleSynchInterface* base, std::unique_ptr<DeviceDriver> executor)
        : m_base{ base }
        , m_executor{ std::move(executor) } {
        if (!m_executor)
            throw std::runtime_error{"synchronous interface initialized "
                                     "with NULL driver handle"};

        ((DeviceInterface*)base)->pimpl->m_owner = base;
        m_executor->initializeDevice();
    }

    ~_impl() {
        m_executor->closeDevice();
    }

    void processAction(AbstractAction::actionHandle_t act)
    try {
        auto res = m_executor->executeAction(std::move(act));
        m_base->notifyOwner(std::move(res));
    }
    catch(driver_error& err) {
        std::cerr << err.what() << std::endl;
    }

    std::unique_ptr<AbstractResponse> getResponse() {
        return std::move(m_resp);
    }

    void responseGetter(std::unique_ptr<AbstractResponse> resp) {
        m_resp.swap(resp);
    }

private:
    std::unique_ptr<AbstractResponse> m_resp;
    SimpleSynchInterface* m_base;
    std::unique_ptr<DeviceDriver> m_executor;

};

class QueuedAsynchInterface::_impl
{
public:
    _impl(QueuedAsynchInterface* base,
          SimpleQueue::handle_t que,
          std::shared_ptr<QueueManager> mgr)
        : m_base{ base }
        , m_queue{ que }
        , m_manager{ mgr } {
        if (!m_queue)
            throw std::runtime_error{"asynchronous interface initialized "
                                     "with NULL queue handle"};
        if (!m_manager)
            throw std::runtime_error{"asynchronous interface initialized "
                                     "with NULL manager handle"};

        m_queue->registerInterface(m_base);
    }

    ~_impl() {
        m_queue->removeInterface(m_base);
    }

    void processAction(AbstractAction::actionHandle_t act)
    {
        auto priAct = dynamic_cast<PriorityAction*>(act.get());

        if (!priAct)
            throw std::runtime_error("interface used with wrong action subtype");

        auto priority = priAct->priority();

        switch(priority) {
        case PriorityAction::First:
            m_queue->push_front(std::move(act));
            break;
        case PriorityAction::Last:
        default:
            m_queue->push_back(std::move(act));
            break;
        }

        if (!m_manager->isWorking())
            m_base->startAsynchQueueProcessing(m_manager);
    }

private:
    QueuedAsynchInterface* m_base;
    SimpleQueue::handle_t m_queue;
    std::shared_ptr<QueueManager> m_manager;

};

void LogicDevice::setInterface(DeviceInterface::interfaceHandle_t rhs)
{
    pimpl->setInterface(rhs);
}

DeviceInterface::interfaceHandle_t LogicDevice::interface() const
{
    return pimpl->interface();
}

void DeviceInterface::notifyOwner(AbstractResponse::responseHandle_t resp)
{
    pimpl->notifyOwner(std::move(resp));
}

void DeviceInterface::execute(AbstractAction::actionHandle_t act)
{
    pimpl->execute(std::move(act));
}

SimpleSynchInterface::SimpleSynchInterface(std::unique_ptr<DeviceDriver> exc)
    : pimpl{ std::make_unique<_impl>(this, std::move(exc)) }
{

}

void SimpleSynchInterface::processAction(AbstractAction::actionHandle_t act)
{
    pimpl->processAction(std::move(act));
}

std::unique_ptr<AbstractResponse> SimpleSynchInterface::getResponse()
{
    return pimpl->getResponse();
}

void SimpleSynchInterface::responseGetter(std::unique_ptr<AbstractResponse> resp)
{
    pimpl->responseGetter(std::move(resp));
}

QueuedAsynchInterface::QueuedAsynchInterface(SimpleQueue::handle_t que,
                                             std::shared_ptr<QueueManager> mgr)
    : pimpl{ std::make_unique<_impl>(this, que, mgr)}
{

}

void QueuedAsynchInterface::processAction(AbstractAction::actionHandle_t act)
{
    pimpl->processAction(std::move(act));
}

DAL_PIMPL_THIS_CONSTRUCTOR(LogicDevice)
DAL_PIMPL_THIS_CONSTRUCTOR(DeviceInterface)
DAL_PIMPL_DEFAULT_DESTRUCTOR(LogicDevice)
DAL_PIMPL_DEFAULT_DESTRUCTOR(DeviceInterface)
DAL_PIMPL_DEFAULT_DESTRUCTOR(SimpleSynchInterface)
DAL_PIMPL_DEFAULT_DESTRUCTOR(QueuedAsynchInterface)
