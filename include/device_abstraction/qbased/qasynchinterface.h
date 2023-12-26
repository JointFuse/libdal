#ifndef QASYNCHINTERFACE_H
#define QASYNCHINTERFACE_H

#include <QObject>

#include "../devicexternal.h"

class QAsynchInterface : public QObject, public QueuedAsynchInterface
{
    Q_OBJECT

public slots:
    void responseReciever(AbstractResponse*);

public:
    QAsynchInterface(ActionQueue::handle_t,
                     std::shared_ptr<QueueManager>,
                     QObject* parent = nullptr);

protected:
    void startAsynchQueueProcessing(std::shared_ptr<QueueManager>) override;

};

#endif // QASYNCHINTERFACE_H
