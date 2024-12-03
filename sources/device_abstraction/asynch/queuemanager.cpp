#include "queuemanager.h"

#include <list>
#include <stdexcept>
#include <iostream>
#include <atomic>
#include <future>
#include <thread>
#include <mutex>
#include <QDebug>

#include "../devicexternal.h"

//#define TIMINGTEST
#ifdef TIMINGTEST
#include <chrono>
#include <math.h>
#endif

using namespace dal;

class QueueManager::_impl
{
    friend QueueManager;

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

    std::unique_ptr<AbstractResponse> exec(
        const std::unique_ptr<AbstractAction>& act) {
        if (m_executor)
            return m_executor->executeAction(act);
        else
            return {};
    }

public:
    SimpleQueue::handle_t m_queue;
    std::atomic<bool> m_isWorking;

private:
    std::unique_ptr<DeviceDriver> m_executor;

};

class SimpleManager::_impl
{
public:
    _impl(SimpleManager* base)
        : m_base{ base } {

    }

    void processQueue()
    try {
        if (!m_processLock.try_lock())
            return;

        m_base->QueueManager::pimpl->m_isWorking = true;

#ifdef TIMINGTEST
        auto durations = std::vector<std::chrono::high_resolution_clock::duration::rep>{};
#endif
        auto act = AbstractAction::actionHandle_t{ nullptr };

        while (m_base->QueueManager::pimpl->m_queue->pop_front(act))
        {
            if (!act ||
                (act->requestor() != nullptr &&
                 !m_base->QueueManager::pimpl->m_queue->tryLockInterface(
                     act->requestor())))
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
                if (act->requestor())
                    m_base->QueueManager::pimpl->m_queue->unlockInterface(
                        act->requestor());

                std::cerr << e.what() << std::endl;
                throw e;
            }
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

        m_processLock.unlock();
        m_base->QueueManager::pimpl->m_isWorking = false;
    }
    catch(driver_error& err) {
        std::cerr << err.what() << std::endl;
        m_processLock.unlock();
        processQueue();
    }
    catch(std::exception& e){
        m_base->QueueManager::pimpl->m_isWorking = false;
        std::cerr << e.what() << std::endl;
        m_processLock.unlock();
        throw e;
    }

    void processAction(AbstractAction::actionHandle_t& act) {
        auto res = m_base->QueueManager::pimpl->exec(act);

        if (res && act->requestor())
            m_base->sendClientResponse(std::move(res));
    }

    void sendClientResponse(AbstractResponse::responseHandle_t act)
    {
        auto cli = act->requestor();
        cli->notifyOwner(std::move(act));
        // here we must free locked interface
        m_base->QueueManager::pimpl->m_queue->unlockInterface(cli);
    }

private:
    SimpleManager* m_base;
    std::mutex m_processLock;

};

class AsynchRespondManager::_impl
{
public:
    _impl(AsynchRespondManager* base)
        : m_base{ base } {

    }

    ~_impl() {
        m_flag = false;

        if (m_task.valid())
            m_task.get();

        for (auto& resp : m_resp)
            m_base->QueueManager::pimpl->m_queue->unlockInterface
                (resp->requestor());
    }

    void sendClientResponse(AbstractResponse::responseHandle_t res) {
        const std::lock_guard<std::mutex> _{ m_mtx };
        m_resp.push_back(std::move(res));

        if (m_taskStarted)
            return;
        else
            m_taskStarted = true;

        m_task = std::async(std::launch::async,
                            [this, respSend = m_base->responseSender()]() {
            while (m_flag) {
                if (!m_resp.empty() && m_mtx.try_lock())
                {
                    const auto cli = m_resp.front()->requestor();
                    respSend(std::move(m_resp.front()));

                    // here we unlocking requestor
                    m_base->QueueManager::pimpl->m_queue->unlockInterface(cli);

                    m_resp.pop_front();
                    m_mtx.unlock();
                }
                else
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

private:
    std::list<AbstractResponse::responseHandle_t> m_resp;
    std::future<void> m_task;
    bool m_taskStarted{ false };
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

std::unique_ptr<DeviceDriver> QueueManager::takeExecutor()
{
    return std::move(pimpl->m_executor);
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

void SimpleManager::sendClientResponse(AbstractResponse::responseHandle_t act)
{
    pimpl->sendClientResponse(std::move(act));
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
