#ifndef QSIMPLEMANAGER_H
#define QSIMPLEMANAGER_H

#include <QObject>

#include "../asynch/queuemanager.h"

class QBaseManager : public QObject, public AsynchRespondManager
{
    Q_OBJECT

public slots:
    void invocationSlot();

public:
    using AsynchRespondManager::AsynchRespondManager;

};

class QSimpleManager : public QBaseManager
{
    Q_OBJECT

public:
    using QBaseManager::QBaseManager;

protected:
    void responseSender(AbstractResponse::responseHandle_t);

};

class QPromiseManager : public QBaseManager
{
    Q_OBJECT

public:
    using QBaseManager::QBaseManager;

protected:
    void responseSender(AbstractResponse::responseHandle_t);

};

#endif // QSIMPLEMANAGER_H
