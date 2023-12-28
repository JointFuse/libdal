#include "qasynchinterface.h"

#include <QMetaObject>

#include "qsimplemanager.h"

void QAsynchInterface::responseReciever(AbstractResponse* resp)
{
    // WARNING The paradigm of this architecture implies a one-to-one
    // correspondence between the response object and the client
    // interface, and thanks to this formal agreement we can afford
    // to pack the pointer into a smart wrapper. But this does not
    // guarantee the occurrence of logical errors in the code leading
    // to the transfer of memory control to more than one smart
    // pointer, which causes undefined behavior
    notifyOwner(AbstractResponse::responseHandle_t{ resp });
}

QAsynchInterface::QAsynchInterface(
    SimpleQueue::handle_t que,
    std::shared_ptr<QueueManager> mgr,
    QObject* parent)
    : QObject{ parent }
    , QueuedAsynchInterface{ que, mgr }
{

}

void QAsynchInterface::startAsynchQueueProcessing(std::shared_ptr<QueueManager> mgr)
{
    QMetaObject::invokeMethod(
        (QSimpleManager*)mgr.get(),
        "invocationSlot",
        Qt::QueuedConnection
    );
}
