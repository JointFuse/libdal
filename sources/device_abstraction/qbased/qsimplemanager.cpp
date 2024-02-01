#include "qsimplemanager.h"

#include <type_traits>
#include <QMetaObject>
#include <QDebug>
#include <QThread>

#include "qasynchinterface.h"
#include "../material/promise.h"

void QBaseManager::invocationSlot()
{
    processQueue();
}

void QSimpleManager::responseSender(AbstractResponse::responseHandle_t resp)
{
    auto cli = (QAsynchInterface*)resp->requestor();

    QMetaObject::invokeMethod(
        cli,
        "responseReciever",
        Qt::QueuedConnection,
        // WARNING QT expects an argument of a type that
        // supports copying, so it has to get rid of the
        // smart pointer wrapper, which potentially leads
        // to a memory leak
        Q_ARG(AbstractResponse*, resp.release())
    );
}

void QPromiseManager::responseSender(AbstractResponse::responseHandle_t resp)
{
    auto promResp = (PromiseResponse*)resp.get();
    auto prom = decltype(promResp->promise){ std::move(promResp->promise) };
    prom.set_value(std::move(resp));
}

