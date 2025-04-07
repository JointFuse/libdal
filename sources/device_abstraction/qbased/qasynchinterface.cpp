#include "qasynchinterface.h"

#include <future>

#include <QMetaObject>
#include <QDebug>
#include <QCoreApplication>

#include "qsimplemanager.h"

using namespace dal;

class QBaseInterface::_impl
{
public:
    void startAsynchQueueProcessing(std::shared_ptr<QueueManager> mgr)
    {
        auto qbased = dynamic_cast<QBaseManager*>(mgr.get());

        if (!qbased)
            throw std::runtime_error("invalid qbase manager subtype");

        QMetaObject::invokeMethod
            (qbased, "invocationSlot", Qt::QueuedConnection);
    }

};

namespace AsynchInterface{
thread_local std::list<QAsynchInterface*> Destructable;
}

class QAsynchInterface::_impl
{
public:
    _impl(QAsynchInterface* base) : m_base{ base } {

    }

    ~_impl() {
        m_destructableInterface.push_front(m_base);
        m_base->stopFurtherResponseProcessing();

        auto destructionPreparation = std::async(
            std::launch::async,
            [&inf = *m_base]() {
                inf.removeInterfaceFromQueueBeforeDestruction();
            });

        while(destructionPreparation.wait_for(std::chrono::seconds(0)) !=
              std::future_status::ready)
            qApp->processEvents();

        m_destructableInterface.pop_front();
    }

    void responseReciever(AbstractResponse* resp) {
        if (!m_base->queue().checkAliveAndUnlockInterface(resp->requestor())) {
            delete resp;
            return;
        }

        if (0 < m_destructableInterface.size() &&
            m_destructableInterface.back() != m_base) {

            if (m_base->queue().tryLockInterface(resp->requestor())) {
                QMetaObject::invokeMethod(
                    m_base,
                    "responseReciever",
                    Qt::QueuedConnection,
                    // WARNING QT expects an argument of a type that
                    // supports copying, so it has to get rid of the
                    // smart pointer wrapper, which potentially leads
                    // to a memory leak
                    Q_ARG(dal::AbstractResponse*, resp)
                    );
            }

            return;
        }

        // WARNING The paradigm of this architecture implies a one-to-one
        // correspondence between the response object and the client
        // interface, and thanks to this formal agreement we can afford
        // to pack the pointer into a smart wrapper. But this does not
        // guarantee the occurrence of logical errors in the code leading
        // to the transfer of memory control to more than one smart
        // pointer, which causes undefined behavior
        m_base->notifyOwner(AbstractResponse::responseHandle_t{ resp });
    }

private:
    QAsynchInterface* m_base;

    inline static std::list<QAsynchInterface*>&
        m_destructableInterface{ AsynchInterface::Destructable };

};

QBaseInterface::QBaseInterface(
    SimpleQueue::handle_t que,
    std::shared_ptr<QueueManager> mgr)
    : QueuedAsynchInterface{ que, mgr }
    , pimpl{ std::make_unique<_impl>() }
{

}

void QBaseInterface::startAsynchQueueProcessing(std::shared_ptr<QueueManager> mgr)
{
    pimpl->startAsynchQueueProcessing(std::move(mgr));
}

void QAsynchInterface::responseReciever(AbstractResponse* resp)
{
    pimpl->responseReciever(resp);
}

QAsynchInterface::QAsynchInterface(
    SimpleQueue::handle_t que,
    std::shared_ptr<QueueManager> mgr)
    : QBaseInterface{ que, mgr }
    , pimpl{ std::make_unique<_impl>(this) }
{

}

DAL_PIMPL_DEFAULT_DESTRUCTOR(QBaseInterface)
DAL_PIMPL_DEFAULT_DESTRUCTOR(QAsynchInterface)
