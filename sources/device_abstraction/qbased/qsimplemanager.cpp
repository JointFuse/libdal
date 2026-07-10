#include "qsimplemanager.h"

#include <type_traits>
#include <QMetaObject>
#include <QDebug>
#include <QThread>

#include "qasynchinterface.h"
#include "../material/promise.h"

using namespace dal;

class QBaseManager::_impl
{
public:
    _impl(QBaseManager* base) : m_base{ base } {

    }

    void processQueue() {
        m_base->processQueue();
    }

private:
    QBaseManager* m_base;

};

class QSimpleManager::_impl
{
public:
    _impl(QSimpleManager* base)
        : m_base{ base }
        , m_infSynchPtr{ std::make_shared<InterfaceCallbackSynchronizationPrimitive>() }
        , m_infSynch{ *m_infSynchPtr } {

    }

    ~_impl() {
        auto interfacesToRelease = m_infSynch.release();
        decltype(auto) queue = m_base->queue();

        for (auto& interface : interfacesToRelease) {
            if (queue.isAlive(interface))
                queue.checkAliveAndUnlockInterface(interface);
        }
    }

    void processAction(AbstractAction::actionHandle_t& act)
    try {
        auto res = m_base->exec(act);

        if (res && act->requestor())
            m_base->sendClientResponse(std::move(res));
    }
    catch(std::exception& exc) {
        if (act->requestor())
            m_infSynch.release(act->requestor());

        throw exc;
    }

    bool takeActionFromQueue(AbstractAction::actionHandle_t& act) {
        act.reset();

        while (m_base->queue().pop_front(act)) {
            if (act && act->requestor() != nullptr &&
                 !m_base->queue().tryLockInterface(act->requestor()))
                continue;
            else
                break;
        }

        if (!act)
            return false;
        else if (act->requestor() == nullptr)
            return true;

        m_infSynch.acquire(act->requestor());
        return true;
    }

    std::function<void(AbstractResponse::responseHandle_t)> responseSender() {
        return [&queue = m_base->queue(),
                &infSynch = *m_infSynchPtr,
                infSynchPtr = m_infSynchPtr](auto resp) {
            auto cli = dynamic_cast<QAsynchInterface*>(resp->requestor());

            if (!cli || infSynch.released())
                return;

            infSynch.release(cli);
            QMetaObject::invokeMethod(
                cli,
                "responseReciever",
                Qt::QueuedConnection,
                // WARNING QT expects an argument of a type that
                // supports copying, so it has to get rid of the
                // smart pointer wrapper, which potentially leads
                // to a memory leak
                Q_ARG(dal::AbstractResponse*, resp.release())
                );
        };
    }

private:
    QSimpleManager* m_base;
    std::shared_ptr<InterfaceCallbackSynchronizationPrimitive> m_infSynchPtr;
    InterfaceCallbackSynchronizationPrimitive& m_infSynch;

};

class QPromiseManager::_impl
{
public:
    std::function<void(AbstractResponse::responseHandle_t)> responseSender() {
        return [](auto resp) {
            auto promResp = dynamic_cast<PromiseResponse*>(resp.get());

            if (!promResp)
                return;

            auto prom = decltype(promResp->promise){ std::move(promResp->promise) };
            prom.set_value(std::move(resp));
        };
    }
};

void QBaseManager::invocationSlot()
{
    pimpl->processQueue();
}

QBaseManager::QBaseManager(std::unique_ptr<DeviceDriver> executor,
                           SimpleQueue::handle_t queue)
    : AsynchRespondManager{ std::move(executor), queue }
    , pimpl{ std::make_unique<_impl>(this) }
{

}

QSimpleManager::QSimpleManager(std::unique_ptr<DeviceDriver> executor,
                               SimpleQueue::handle_t queue)
    : QBaseManager{ std::move(executor), queue }
    , pimpl{ std::make_unique<_impl>(this) }
{

}

void QSimpleManager::processAction(AbstractAction::actionHandle_t& act)
{
    pimpl->processAction(act);
}

bool QSimpleManager::takeActionFromQueue(AbstractAction::actionHandle_t& act)
{
    return pimpl->takeActionFromQueue(act);
}

std::function<void(AbstractResponse::responseHandle_t)> QSimpleManager::responseSender()
{
    return pimpl->responseSender();
}

QPromiseManager::QPromiseManager(std::unique_ptr<DeviceDriver> executor,
                                 SimpleQueue::handle_t queue)
    : QBaseManager{ std::move(executor), queue }
    , pimpl{ std::make_unique<_impl>() }
{

}

std::function<void(AbstractResponse::responseHandle_t)> QPromiseManager::responseSender()
{
    return pimpl->responseSender();
}

DAL_PIMPL_DEFAULT_DESTRUCTOR(QBaseManager)
DAL_PIMPL_DEFAULT_DESTRUCTOR(QSimpleManager)
DAL_PIMPL_DEFAULT_DESTRUCTOR(QPromiseManager)
