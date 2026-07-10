#ifndef DAL_INTEGRATION_H
#define DAL_INTEGRATION_H

#include <QMetaType>
#include <QObject>
#include <QThread>
#include <atomic>
#include <condition_variable>
#include <device_abstraction/asynch/queuemanager.h>
#include <device_abstraction/devicexternal.h>
#include <device_abstraction/material/promise.h>
#include <future>
#include <mutex>
#include <queue>

//------------------------------------------------------------------------------
/**
 * @brief The SafeFutureChannel class
 * @abstract передает std::future<dal::AbstractResponse::responseHandle_t>
 *  из потока клиента в поток QDalResponseReader
 */
class SafeFutureChannel final
{
  public:
    ~SafeFutureChannel();
    /**
     * @brief take
     * @return future содержащий shared state до вызова close(),
     *  иначе future без shared state
     */
    std::future<dal::AbstractResponse::responseHandle_t> take();
    void put( std::future<dal::AbstractResponse::responseHandle_t>&& );

    void close() { m_workFlag.store( false, std::memory_order_release ); }

  private:
    std::queue<std::future<dal::AbstractResponse::responseHandle_t>> m_futureQueue;
    std::condition_variable m_queueConditionVariable;
    std::atomic_bool m_workFlag{ true };
    std::mutex m_queueMutex;
};

//------------------------------------------------------------------------------
/**
 * @brief The QDalClient class
 * @abstract базовый класс для безопасной интеграции libdal в проект на базе Qt
 */
class QDalClient : public QObject, public dal::LogicDevice
{
    Q_OBJECT

  public:
    /**
     * @brief The DeviceHandle class
     * @abstract ручка содержащая все необходимое для управления dal::DeviceDriver
     *  управляет временем жизни dal::DeviceDriver, dal::QueueManager и тд
     */
    class DeviceHandle
    {
        friend class QDalClient;

      public:
        ~DeviceHandle();
        void executeAction( std::unique_ptr<dal::PromiseAction> );
        void clearActionQueue();

      private:
        dal::QueueManager::managerHandle_t m_manager;
        dal::DeviceInterface::interfaceHandle_t m_interface;
        dal::QueueInterface::handle_t m_queue;
        std::shared_ptr<SafeFutureChannel> m_futureQueue;
        std::unique_ptr<QThread> m_deviceThread;
    };

  signals:
    void gotResponse( std::shared_ptr<dal::AbstractResponse> );

  public:
    QDalClient();
    ~QDalClient();

    /**
     * @brief createDevice
     * @abstract перемещает dal::DeviceDriver в рабочий поток, инициализирует
     *  всю обвязку
     * @param deviceThread - клиент может предоставить экземпляр потока
     *  работы dal::DeviceDriver
     * @return интерфейс к готовому для использования драйверу
     */
    std::shared_ptr<DeviceHandle> createDevice(
        dal::DeviceDriver::driverHandle_t&&, std::unique_ptr<QThread>&& deviceThread = {} );

  private:
    void responseGetter( dal::AbstractResponse::responseHandle_t ) override {}

  private:
    QThread m_responseReaderThread;
    std::shared_ptr<SafeFutureChannel> m_futureQueue;
};

//------------------------------------------------------------------------------
/**
 * @brief The QDalResponseReader class
 * @abstract ждет результат выполнения dal::DeviceDriver и безопасно передает
 *  в клиентский поток через SIGNAL/SLOT
 */
class QDalResponseReader final : public QObject
{
    Q_OBJECT

  signals:
    void signalGotResponse( std::shared_ptr<dal::AbstractResponse> );

  public slots:
    void exec();

  public:
    QDalResponseReader( std::shared_ptr<SafeFutureChannel> );

  private:
    std::shared_ptr<SafeFutureChannel> m_futureQueue;
};

Q_DECLARE_METATYPE( std::shared_ptr<dal::AbstractResponse> )

#endif    // DAL_INTEGRATION_H
