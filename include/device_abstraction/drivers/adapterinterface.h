#ifndef ADAPTERINTERFACE_H
#define ADAPTERINTERFACE_H

#include "../material/actionabstract.h"
#include "../material/responseabstract.h"

#include <stdexcept>

namespace dal {    ////////////////////////////////////////////////////////////////
/**
 * @brief The driver_error class
 */
class driver_error : public std::runtime_error
{
  public:
    using std::runtime_error::runtime_error;
};
/**
 * @brief The DeviceDriver class
 */
class DeviceDriver
{
  public:
    using driverHandle_t = std::unique_ptr<DeviceDriver>;

  public:
    virtual ~DeviceDriver() = default;

    virtual void initializeDevice() = 0;
    virtual void closeDevice()      = 0;

    virtual AbstractResponse::responseHandle_t
    executeAction( const AbstractAction::actionHandle_t& ) = 0;
};

}    // namespace dal

#endif    // ADAPTERINTERFACE_H
