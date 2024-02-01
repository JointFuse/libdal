#ifndef QSIMPLEMANAGER_H
#define QSIMPLEMANAGER_H

#include <QObject>

#include "../dalcore.h"
#include "../asynch/queuemanager.h"

namespace dal { ////////////////////////////////////////////////////////////////
/**
 * @brief The QBaseManager class
 */
class QBaseManager : public QObject, public AsynchRespondManager
{
    Q_OBJECT

public slots:
    void invocationSlot();

public:
    QBaseManager(std::unique_ptr<DeviceDriver> executor,
                 SimpleQueue::handle_t queue);
    ~QBaseManager();

private:
    DAL_DECLARE_PIMPL

};
/**
 * @brief The QSimpleManager class
 */
class QSimpleManager : public QBaseManager
{
    Q_OBJECT

public:
    QSimpleManager(std::unique_ptr<DeviceDriver> executor,
                   SimpleQueue::handle_t queue);
    ~QSimpleManager();

protected:
    void responseSender(AbstractResponse::responseHandle_t);

private:
    DAL_DECLARE_PIMPL

};
/**
 * @brief The QPromiseManager class
 */
class QPromiseManager : public QBaseManager
{
    Q_OBJECT

public:
    QPromiseManager(std::unique_ptr<DeviceDriver> executor,
                    SimpleQueue::handle_t queue);
    ~QPromiseManager();

protected:
    void responseSender(AbstractResponse::responseHandle_t);

private:
    DAL_DECLARE_PIMPL

};

} //////////////////////////////////////////////////////////////////////////////

#endif // QSIMPLEMANAGER_H
