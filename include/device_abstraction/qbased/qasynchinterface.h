#ifndef QASYNCHINTERFACE_H
#define QASYNCHINTERFACE_H

#include <QObject>
#include <QMetaType>

#include "../devicexternal.h"

Q_DECLARE_METATYPE(AbstractResponse*)

class QBaseInterface : public QueuedAsynchInterface
{
public:
    QBaseInterface(SimpleQueue::handle_t,
                   std::shared_ptr<QueueManager>);

protected:
    void startAsynchQueueProcessing(std::shared_ptr<QueueManager>) override;

};

class QAsynchInterface : public QObject, public QBaseInterface
{
    Q_OBJECT

public slots:
    void responseReciever(AbstractResponse*);

public:
    using QBaseInterface::QBaseInterface;

};

#endif // QASYNCHINTERFACE_H
