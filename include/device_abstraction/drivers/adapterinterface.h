#ifndef ADAPTERINTERFACE_H
#define ADAPTERINTERFACE_H

#include <stdexcept>

#include "../material/actionabstract.h"
#include "../material/responseabstract.h"

namespace dal { ////////////////////////////////////////////////////////////////
/**
 * @brief The driver_error class
 */
class driver_error : public std::runtime_error { };
/**
 * @brief The DeviceDriver class
 */
class DeviceDriver
{
public:
    using driverHandle_t = std::unique_ptr<DeviceDriver>;

public:
    virtual ~DeviceDriver() = default;

    virtual void initializeDevice   () = 0;
    virtual void closeDevice        () = 0;

    virtual AbstractResponse::responseHandle_t executeAction(
                const AbstractAction::actionHandle_t&) = 0;

};

} //////////////////////////////////////////////////////////////////////////////

#endif // ADAPTERINTERFACE_H
