#ifndef QSIMPLEMANAGER_H
#define QSIMPLEMANAGER_H

#include <QObject>

#include "../asynch/queuemanager.h"

class QSimpleManager : public QObject, public AsynchRespondManager
{
    Q_OBJECT

public slots:
    void invocationSlot();

public:
    using AsynchRespondManager::AsynchRespondManager;

protected:
    void responseSender(AbstractResponse::responseHandle_t);

};

#endif // QSIMPLEMANAGER_H
