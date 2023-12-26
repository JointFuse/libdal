#ifndef ADAPTERINTERFACE_H
#define ADAPTERINTERFACE_H

#include <stdexcept>

#include "../material/actionabstract.h"
#include "../material/responseabstract.h"

class driver_error : public std::runtime_error { };

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

#endif // ADAPTERINTERFACE_H
