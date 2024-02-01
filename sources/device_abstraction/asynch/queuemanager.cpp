#include "queuemanager.h"

#include <list>
#include <stdexcept>
#include <iostream>
#include <atomic>
#include <future>
#include <thread>
#include <mutex>
#include <QDebug>

//#define TIMINGTEST
#ifdef TIMINGTEST
#include <chrono>
#include <math.h>
#endif

using namespace dal;

class QueueManager::_impl
{
public:
    _impl(std::unique_ptr<DeviceDriver> executor,
          SimpleQueue::handle_t queue)
        : m_executor{ std::move(executor) }
        , m_queue{ queue }
        , m_isWorking{ false } {
        m_executor->initializeDevice();
    }

    ~_impl() {
        m_executor->closeDevice();
    }

    bool isWorking() const noexcept {
        return m_isWorking;
    }

public:
    std::unique_ptr<DeviceDriver> m_executor;
    SimpleQueue::handle_t m_queue;
    std::atomic<bool> m_isWorking;

};

class SimpleManager::_impl
{
public:
    _impl(SimpleManager* base)
        : m_base{ base } {

    }

    void processQueue()
    try {
        m_base->QueueManager::pimpl->m_isWorking = true;

#ifdef TIMINGTEST
        auto durations = std::vector<std::chrono::high_resolution_clock::duration::rep>{};
#endif
        while (m_base->QueueManager::pimpl->m_queue->queueSize())
        {
            auto act{ m_base->QueueManager::pimpl->m_queue->pop_front() };

            if (act->requestor() != nullptr &&
                (!m_base->QueueManager::pimpl->m_queue->hasInterface(act->requestor()) ||
                 !m_base->QueueManager::pimpl->m_queue->tryLockInterface(act->requestor())))
                continue;

            try {
#ifdef TIMINGTEST
                const auto start = std::chrono::high_resolution_clock::now();
#endif
                m_base->processAction(act);
#ifdef TIMINGTEST
                durations.push_back((std::chrono::high_resolution_clock::now() - start).count());
#endif
            }
            catch(std::exception& e) {
                m_base->QueueManager::pimpl->m_queue->unlockInterface(act->requestor());
                throw e;
            }

            m_base->QueueManager::pimpl->m_queue->unlockInterface(act->requestor());
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

        m_base->QueueManager::pimpl->m_isWorking = false;
    }
    catch(driver_error& err) {
        std::cerr << err.what() << std::endl;
        processQueue();
    }
    catch(std::exception& e){
        m_base->QueueManager::pimpl->m_isWorking = false;
        throw e;
    }

    void processAction(AbstractAction::actionHandle_t& act) {
        auto res = m_base->QueueManager::pimpl->m_executor->executeAction(act);
        m_base->sendClientResponse(std::move(res));
    }

private:
    SimpleManager* m_base;

};

class AsynchRespondManager::_impl
{
public:
    _impl(AsynchRespondManager* base)
        : m_base{ base } {
        m_task = std::async(std::launch::async, [this]() {
            while (m_flag) {
                if (!m_resp.empty() && m_mtx.try_lock())
                {
                    m_base->responseSender(std::move(m_resp.front()));
                    m_resp.pop_front();
                    m_mtx.unlock();
                }
                else
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    ~_impl() {
        m_flag = false;
    }

    void sendClientResponse(AbstractResponse::responseHandle_t res) {
        const std::lock_guard<std::mutex> _{ m_mtx };
        m_resp.push_back(std::move(res));
    }

private:
    std::list<AbstractResponse::responseHandle_t> m_resp;
    std::future<void> m_task;
    std::atomic<bool> m_flag{ true };
    std::mutex m_mtx;
    AsynchRespondManager* m_base;

};

QueueManager::QueueManager(std::unique_ptr<DeviceDriver> executor,
                           SimpleQueue::handle_t queue)
    : pimpl { std::make_unique<_impl>(std::move(executor), queue)}
{

}

bool QueueManager::isWorking() const
{
    return pimpl->isWorking();
}

SimpleManager::SimpleManager(std::unique_ptr<DeviceDriver> executor,
                             SimpleQueue::handle_t queue)
    : QueueManager{ std::move(executor), queue }
    , pimpl{ std::make_unique<_impl>(this) }
{

}

void SimpleManager::processQueue()
{
    pimpl->processQueue();
}

void SimpleManager::processAction(AbstractAction::actionHandle_t& act)
{
    pimpl->processAction(act);
}

AsynchRespondManager::AsynchRespondManager(std::unique_ptr<DeviceDriver> executor,
                                           SimpleQueue::handle_t queue)
    : SimpleManager{ std::move(executor), queue }
    , pimpl{ std::make_unique<_impl>(this) }
{

}

void AsynchRespondManager::sendClientResponse(AbstractResponse::responseHandle_t res)
{
    pimpl->sendClientResponse(std::move(res));
}

DAL_PIMPL_DEFAULT_DESTRUCTOR(QueueManager)
DAL_PIMPL_DEFAULT_DESTRUCTOR(SimpleManager)
DAL_PIMPL_DEFAULT_DESTRUCTOR(AsynchRespondManager)
