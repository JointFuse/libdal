#ifndef QASYNCHINTERFACE_H
#define QASYNCHINTERFACE_H

#include <QObject>
#include <QMetaType>

#include "../devicexternal.h"

Q_DECLARE_METATYPE(AbstractResponse*)

class QAsynchInterface : public QObject, public QueuedAsynchInterface
{
    Q_OBJECT

public slots:
    void responseReciever(AbstractResponse*);

public:
    QAsynchInterface(SimpleQueue::handle_t,
                     std::shared_ptr<QueueManager>,
                     QObject* parent = nullptr);

protected:
    void startAsynchQueueProcessing(std::shared_ptr<QueueManager>) override;

};

#endif // QASYNCHINTERFACE_H
