#include "queuemanager.h"

#include <stdexcept>
#include <iostream>

QueueManager::QueueManager(
    std::unique_ptr<DeviceDriver> executor,
    ActionQueue::handle_t queue)
    : m_executor{ std::move(executor) }
    , m_queue{ queue }
    , m_isWorking{ false }
{
    m_executor->initializeDevice();
}

QueueManager::~QueueManager()
{
    m_executor->closeDevice();
}

//#define TIMINGTEST
#ifdef TIMINGTEST
#include <chrono>
#include <math.h>
#include <QDebug>
#endif

void SimpleManager::processQueue()
try {
    m_isWorking = true;

#ifdef TIMINGTEST
    auto durations = std::vector<std::chrono::high_resolution_clock::duration::rep>{};
#endif
    while (m_queue->queueSize())
    {
        auto act{ m_queue->pop_front() };

        if (!m_queue->hasInterface(act->requestor()) ||
            !m_queue->tryLockInterface(act->requestor()))
            continue;

        try {
            auto res = m_executor->executeAction(act);
#ifdef TIMINGTEST
            const auto start = std::chrono::high_resolution_clock::now();
#endif
            sendClientResponse(std::move(res));
#ifdef TIMINGTEST
            durations.push_back((std::chrono::high_resolution_clock::now() - start).count());
#endif
        }
        catch(std::exception& e) {
            m_queue->unlockInterface(act->requestor());
            throw e;
        }

        m_queue->unlockInterface(act->requestor());
    }

#ifdef TIMINGTEST
    auto avarage = double(.0);
    for (auto val : durations)
        avarage += val;
    avarage /= durations.size();
    avarage /=  std::pow(10, 3);
    if (!isnan(avarage))
        qInfo() << "Avarage manager processing: " << avarage << "mcS\n";
#endif

    m_isWorking = false;
}
catch(driver_error& err) {
    std::cerr << err.what() << std::endl;
    processQueue();
}
catch(std::exception& e){
    m_isWorking = false;
    throw e;
}

AsynchRespondManager::AsynchRespondManager(std::unique_ptr<DeviceDriver> executor,
                                           ActionQueue::handle_t queue)
    : SimpleManager{ std::move(executor), queue }
{
    m_task = std::async(std::launch::async, [this]() {
        while (m_flag) {
            if (!m_resp.empty() && m_mtx.try_lock())
            {
                responseSender(std::move(m_resp.front()));
                m_resp.pop_front();
                m_mtx.unlock();
            }
            else
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
}

AsynchRespondManager::~AsynchRespondManager()
{
    m_flag = false;
}

void AsynchRespondManager::sendClientResponse(AbstractResponse::responseHandle_t res)
{
    const std::lock_guard<std::mutex> _{ m_mtx };
    m_resp.push_back(std::move(res));
}
