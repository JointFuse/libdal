#ifndef ASYNCHMGRTEST_H
#define ASYNCHMGRTEST_H

#include <memory>

#include <QObject>
#include <QThread>

class AbstractionTestDevice;

class AsynchMgrTest : public QObject
{
    Q_OBJECT

public slots:
    void test();

public:
    AsynchMgrTest(QObject* parent = nullptr);
    ~AsynchMgrTest();

    void threads();

    bool finished{ false };

private:
    AbstractionTestDevice* m_dvc;
    QThread m_thread;
    void* mgr;
    void* inf;

};

#endif // ASYNCHMGRTEST_H
