#ifndef QASYNCHINTERFACE_H
#define QASYNCHINTERFACE_H

#include <QObject>
#include <QMetaType>

#include "../dalcore.h"
#include "../devicexternal.h"

Q_DECLARE_METATYPE(dal::AbstractResponse*)

namespace dal { ////////////////////////////////////////////////////////////////
/**
 * @brief The QBaseInterface class
 */
class QBaseInterface : public QueuedAsynchInterface
{
public:
    QBaseInterface(SimpleQueue::handle_t,
                   std::shared_ptr<QueueManager>);
    ~QBaseInterface();

protected:
    void startAsynchQueueProcessing(std::shared_ptr<QueueManager>) override;

private:
    DAL_DECLARE_PIMPL

};
/**
 * @brief The QAsynchInterface class
 */
class QAsynchInterface : public QObject, public QBaseInterface
{
    Q_OBJECT

public slots:
    void responseReciever(AbstractResponse*);

public:
    QAsynchInterface(SimpleQueue::handle_t,
                     QueueManager::managerHandle_t);
    ~QAsynchInterface();

private:
    DAL_DECLARE_PIMPL

};

} //////////////////////////////////////////////////////////////////////////////

#endif // QASYNCHINTERFACE_H
