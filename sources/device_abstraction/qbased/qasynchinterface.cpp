#include "qasynchinterface.h"

#include <QMetaObject>
#include <QDebug>

#include "qsimplemanager.h"

using namespace dal;

class QBaseInterface::_impl
{
public:
    void startAsynchQueueProcessing(std::shared_ptr<QueueManager> mgr)
    {
        QMetaObject::invokeMethod(
            (QBaseManager*)mgr.get(),
            "invocationSlot",
            Qt::QueuedConnection
        );
    }

};

class QAsynchInterface::_impl
{
public:
    _impl(QAsynchInterface* base) : m_base{ base } {

    }

    void responseReciever(AbstractResponse* resp)
    {
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
