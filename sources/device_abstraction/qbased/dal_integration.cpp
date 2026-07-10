#include "dal_integration.h"

#include <QDebug>
#include <device_abstraction/qbased/qasynchinterface.h>
#include <device_abstraction/qbased/qsimplemanager.h>

//------------------------------------------------------------------------------

SafeFutureChannel::~SafeFutureChannel()
{
    m_workFlag.store( false, std::memory_order_release );
}

std::future<dal::AbstractResponse::responseHandle_t> SafeFutureChannel::take()
{
    if ( m_workFlag.load( std::memory_order_acquire ) == false )
        return {};

    static constexpr auto TimeoutMS = std::chrono::milliseconds{ 1 };
    auto lock                       = std::unique_lock{ m_queueMutex };

    while ( m_queueConditionVariable.wait_for( lock, TimeoutMS ) == std::cv_status::timeout ||
            m_futureQueue.empty() ) {
        if ( m_workFlag.load( std::memory_order_acquire ) == false )
            return {};
        if ( m_futureQueue.empty() == false )
            break;
    }

    auto future = std::move( m_futureQueue.front() );
    m_futureQueue.pop();
    return future;
}

void SafeFutureChannel::put( std::future<dal::AbstractResponse::responseHandle_t>&& arg )
{
    const auto _ = std::lock_guard{ m_queueMutex };
    m_futureQueue.emplace();
    std::swap( m_futureQueue.back(), arg );
    m_queueConditionVariable.notify_one();
}

//------------------------------------------------------------------------------

QDalClient::DeviceHandle::~DeviceHandle()
{
    m_queue->clear();
    m_deviceThread->exit();
    m_deviceThread->wait();
    m_interface.reset();
}

void QDalClient::DeviceHandle::executeAction( std::unique_ptr<dal::PromiseAction> actPtr )
{
    if ( actPtr.get() == nullptr ) {
        qWarning()
            << "[QDalClient::DeviceHandle][executeAction] Warning : action pointer is nullptr";
        return;
    }

    try {
        m_futureQueue->put( actPtr->promise.get_future() );
        m_interface->execute( std::move( actPtr ) );
    }
    catch ( std::future_error& err ) {
        qWarning() << "[QDalClient::DeviceHandle][executeAction] Warning : future error: "
                   << err.what();
    }
}

void QDalClient::DeviceHandle::clearActionQueue()
{
    m_queue->clear();
}

//------------------------------------------------------------------------------

QDalClient::QDalClient() : m_futureQueue{ std::make_shared<SafeFutureChannel>() }
{
    auto responseReader = new QDalResponseReader{ m_futureQueue };
    responseReader->moveToThread( &m_responseReaderThread );
    QObject::connect(
        &m_responseReaderThread,
        &QThread::finished,
        responseReader,
        &QDalResponseReader::deleteLater,
        Qt::DirectConnection );
    connect(
        &m_responseReaderThread,
        &QThread::started,
        responseReader,
        &QDalResponseReader::exec,
        Qt::QueuedConnection );
    connect(
        responseReader,
        &QDalResponseReader::signalGotResponse,
        this,
        &QDalClient::gotResponse,
        Qt::QueuedConnection );
    m_responseReaderThread.start();
}

QDalClient::~QDalClient()
{
    m_futureQueue->close();
    m_responseReaderThread.exit();
    m_responseReaderThread.wait();
}

std::shared_ptr<QDalClient::DeviceHandle> QDalClient::createDevice(
    dal::DeviceDriver::driverHandle_t&& driver, std::unique_ptr<QThread>&& deviceThread )
{
    auto device      = std::make_shared<DeviceHandle>();
    auto& deviceHndl = *device;

    if ( deviceThread.get() != nullptr )
        deviceHndl.m_deviceThread.swap( deviceThread );
    else
        deviceHndl.m_deviceThread = std::make_unique<QThread>();

    deviceHndl.m_queue = std::make_shared<dal::SimpleQueue>();
    deviceHndl.m_manager =
        std::make_shared<dal::QPromiseManager>( std::move( driver ), deviceHndl.m_queue );
    deviceHndl.m_interface =
        std::make_shared<dal::QBaseInterface>( deviceHndl.m_queue, deviceHndl.m_manager );
    setInterface( deviceHndl.m_interface );
    ( (dal::QPromiseManager*)deviceHndl.m_manager.get() )
        ->moveToThread( deviceHndl.m_deviceThread.get() );
    deviceHndl.m_deviceThread->start();
    deviceHndl.m_futureQueue = m_futureQueue;
    return device;
}

//------------------------------------------------------------------------------

void QDalResponseReader::exec()
{
    if ( m_futureQueue.get() == nullptr )
        return;

    auto& futureQueueHndl = *m_futureQueue;
    auto future           = futureQueueHndl.take();

    for ( ; future.valid(); future = futureQueueHndl.take() ) {
        auto responsePtr = dal::AbstractResponse::responseHandle_t{};

        try {
            responsePtr = future.get();
        }
        catch ( std::future_error& ) {
            continue;
        }
        catch ( std::exception& exc ) {
            qWarning() << "[QDalResponseReader][exec] Warning : " << exc.what();
            continue;
        }

        auto sharedResponse = std::shared_ptr<dal::AbstractResponse>{ responsePtr.release() };
        emit signalGotResponse( sharedResponse );
    }
}

QDalResponseReader::QDalResponseReader( std::shared_ptr<SafeFutureChannel> arg )
    : m_futureQueue{ arg }
{
}

//------------------------------------------------------------------------------
