#include "asynchmgrtest.h"

#include <chrono>
#include <thread>
#include <algorithm>
#include <math.h>

#include <QCoreApplication>
#include <QDebug>

#include "device_abstraction/qbased/qasynchinterface.h"
#include "device_abstraction/qbased/qsimplemanager.h"
#include "device_abstraction/material/actionabstract.h"
#include "device_abstraction/material/promise.h"
#include "device_abstraction/material/responseabstract.h"
#include "device_abstraction/devicexternal.h"
#include "device_abstraction/drivers/adapterinterface.h"

#define PROMISE

using namespace dal;

struct CalcTimeoutAction :
#ifdef PROMISE
                           public PromiseAction
#else
                           public PriorityAction
#endif
{
    CalcTimeoutAction()
    {}

    std::chrono::high_resolution_clock::time_point request_time;
};

struct ExecutionTimeoutResp :
#ifdef PROMISE
                              public PromiseResponse
#else
                              public AbstractResponse
#endif
{
    ExecutionTimeoutResp(CalcTimeoutAction* rhs)
#ifdef PROMISE
        : PromiseResponse{ rhs }
#else
        : AbstractResponse{ rhs->requestor() }
#endif
        , exec_time{ std::chrono::high_resolution_clock::now() }
        , exec_timeout{ exec_time - rhs->request_time }
    {}

    std::chrono::high_resolution_clock::time_point exec_time;
    std::chrono::high_resolution_clock::duration exec_timeout;
};

class TestDriver : public DeviceDriver
{
public:
    void initializeDevice   () {}
    void closeDevice        () {}

    AbstractResponse::responseHandle_t executeAction(
        const AbstractAction::actionHandle_t& act) {
        const auto tm = (CalcTimeoutAction*)act.get();
        auto res = AbstractResponse::responseHandle_t{ new ExecutionTimeoutResp(tm) };
        return res;
    }
};

class TestInterface :
#ifdef PROMISE
                      public QBaseInterface
{
public:
    using QBaseInterface::QBaseInterface;
};
#else
                     public QAsynchInterface
{
public:
    using QAsynchInterface::QAsynchInterface;
};
#endif

class TestManager :
#ifdef PROMISE
                    public QPromiseManager
{
public:
    using QPromiseManager::QPromiseManager;
};
#else
                    public QSimpleManager
{
public:
    using QSimpleManager::QSimpleManager;
};
#endif

class AbstractionTestDevice : public LogicDevice
{
public:
    using LogicDevice::LogicDevice;

    void responseGetter(std::unique_ptr<AbstractResponse> abs_resp) {
        const auto resp = (ExecutionTimeoutResp*)abs_resp.get();
        durations.push_back(resp->exec_timeout.count());
        pts.push_back(resp->exec_time);
        respDurations.push_back((std::chrono::high_resolution_clock::now() - resp->exec_time).count());
    }

    void executeTest() {
        static constexpr auto SELECTIONSIZE = 100000;
        static constexpr auto WAITTIME = 1000;

        for (auto sz = SELECTIONSIZE; sz <= SELECTIONSIZE * 20; sz += SELECTIONSIZE)
        {
            durations.clear();
            pts.clear();
            respDurations.clear();

            durations.reserve(sz);
            pts.reserve(sz);
            respDurations.reserve(sz);

#ifdef PROMISE
            auto ftrBuf = std::vector<std::future<AbstractResponse::responseHandle_t>>{};
            ftrBuf.reserve(sz);
#endif
            auto actBuf = std::vector<AbstractAction::actionHandle_t>{};
            actBuf.reserve(sz);
            for (auto i = 0; i < sz; ++i)
            {
                auto act = new CalcTimeoutAction;
                actBuf.push_back(AbstractAction::actionHandle_t(act));
#ifdef PROMISE
                ftrBuf.push_back(act->promise.get_future());
#endif
            }
            for (auto i = 0; i < sz; ++i)
            {
                const auto time = std::chrono::high_resolution_clock::now();
                ((CalcTimeoutAction*)actBuf[i].get())->request_time = time;
                interface()->execute(std::move(actBuf[i]));
            }
#ifdef PROMISE
            for (auto& ftr : ftrBuf)
            try {
                auto abs_resp = ftr.get();
                const auto resp = (ExecutionTimeoutResp*)abs_resp.get();
                durations.push_back(resp->exec_timeout.count());
                pts.push_back(resp->exec_time);
                respDurations.push_back((std::chrono::high_resolution_clock::now() - resp->exec_time).count());
            }
            catch(...) {

            }
#else
            auto szBuf = 0;
            const auto startPoint = std::chrono::high_resolution_clock::now();
            while (szBuf < sz && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startPoint).count() < 10000)
            {
                qApp->processEvents(QEventLoop::ProcessEventsFlag::WaitForMoreEvents, WAITTIME);
                szBuf = durations.size();
            }
#endif
            auto avarage = double(.0);
            for (auto val : durations)
                avarage += val;
            avarage /= durations.size();
            auto dispersion = double(0);
            for (auto val : durations)
                dispersion += std::pow(val - avarage, 2);
            dispersion /= durations.size() - 1;
            dispersion = std::pow(dispersion, 0.5) / std::pow(10, 6);
            avarage /=  std::pow(10, 6);

            auto hz = double();
            for (auto i = 1; i < pts.size(); ++i)
                hz += (pts[i] - pts[i - 1]).count();
            hz /= std::pow(10, 9);
            hz /= pts.size() - 1;
            hz = 1 / hz;
            hz /= std::pow(10, 3);

            std::sort(durations.begin(), durations.end());
            auto median = durations[durations.size() / 2] / std::pow(10, 6);

            auto avarageRes = double(.0);
            for (auto val : respDurations)
                avarageRes += val;
            avarageRes /= respDurations.size();
            avarageRes /=  std::pow(10, 6);

            qInfo() << "Selection size: " << sz
                    << "N; Processed size: " << durations.size() << "N "
                    << "(" << durations.size() * 100.0 / sz << "%);\n"
                    << "Avarage response duration: " << avarageRes << "ms\n"
                    << "Execution timeout:\n"
                    << "\tavarage:\t" << avarage << "ms\n"
                    << "\tmedian:\t" << median << "ms\n"
                    << "\tdispersion:\t" << dispersion << "ms\n"
                    << "Execution frequency: " << hz << "KHz\n";
        }
    }

private:
    std::vector<decltype(ExecutionTimeoutResp::exec_timeout)::rep> durations;
    std::vector<decltype(ExecutionTimeoutResp::exec_time)> pts;
    std::vector<decltype(ExecutionTimeoutResp::exec_timeout)::rep> respDurations;

};

void AsynchMgrTest::test()
{
    m_thread.start();
    m_dvc->executeTest();
    finished = true;
}

AsynchMgrTest::AsynchMgrTest(QObject* parent)
    : QObject{ parent }
{
    auto queue      = SimpleQueue::handle_t             { new SimpleQueue };
    auto driver     = DeviceDriver::driverHandle_t      { new TestDriver };
    mgr             = new TestManager(std::move(driver), queue);
    auto manager    = QueueManager::managerHandle_t     { (TestManager*)mgr };
    inf             = new TestInterface(queue, manager);
    auto interface  = DeviceInterface::interfaceHandle_t{ (TestInterface*)inf };
    m_dvc           = std::make_unique<AbstractionTestDevice>();
    m_dvc           ->setInterface(interface);
}

AsynchMgrTest::~AsynchMgrTest()
{
    m_thread.exit();
    m_thread.wait();
}

void AsynchMgrTest::threads()
{
    ((TestManager*)  mgr)->moveToThread(&m_thread);
#ifndef PROMISE
    ((TestInterface*)inf)->moveToThread(thread());
#endif
}

