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
    std::function<void(AbstractResponse::responseHandle_t)> responseSender() {
        return [](auto resp) {
            auto cli = dynamic_cast<QAsynchInterface*>(resp->requestor());

            if (!cli)
                /*
                 * thorwing deprecated - requestor may entered destructor and
                 * virtual table isn't available for him
                 */
//                throw std::runtime_error("invalid qbased requestor subtype");
                return;

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
};

class QPromiseManager::_impl
{
public:
    std::function<void(AbstractResponse::responseHandle_t)> responseSender() {
        return [](auto resp) {
            auto promResp = dynamic_cast<PromiseResponse*>(resp.get());

            if (!promResp)
                /*
                 * thorwing deprecated - requestor may entered destructor and
                 * virtual table isn't available for him
                 */
//                throw std::runtime_error("invalid promise response subtype");
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
    , pimpl{ std::make_unique<_impl>() }
{

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
