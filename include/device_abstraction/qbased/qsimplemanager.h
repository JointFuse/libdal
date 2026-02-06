#ifndef QSIMPLEMANAGER_H
#define QSIMPLEMANAGER_H

#include "../asynch/queuemanager.h"
#include "../dalcore.h"

#include <QObject>

namespace dal {    ////////////////////////////////////////////////////////////////
/**
 * @brief The QBaseManager class
 */
class QBaseManager : public QObject, public AsynchRespondManager
{
    Q_OBJECT

  public slots:
    void invocationSlot();

  public:
    QBaseManager( std::unique_ptr<DeviceDriver> executor, SimpleQueue::handle_t queue );
    ~QBaseManager();

  private:
    DAL_DECLARE_PIMPL
};
/**
 * @brief The QSimpleManager class
 */
class [[deprecated(
    "This class is deprecated. Using of it may lead to undefined behavior. Strongly recomend to "
    "use QPromiseManager wich not using unsafe callback mechanism QMetaObject::invokeMethod." )]]
QSimpleManager : public QBaseManager
{
    Q_OBJECT

  public:
    QSimpleManager( std::unique_ptr<DeviceDriver> executor, SimpleQueue::handle_t queue );
    ~QSimpleManager();

  protected:
    void processAction( AbstractAction::actionHandle_t& ) override;
    bool takeActionFromQueue( AbstractAction::actionHandle_t& ) override;
    std::function<void( AbstractResponse::responseHandle_t )> responseSender() override;

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
    QPromiseManager( std::unique_ptr<DeviceDriver> executor, SimpleQueue::handle_t queue );
    ~QPromiseManager();

  protected:
    std::function<void( AbstractResponse::responseHandle_t )> responseSender();

  private:
    DAL_DECLARE_PIMPL
};

}    // namespace dal

#endif    // QSIMPLEMANAGER_H
