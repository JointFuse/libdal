#ifndef ACTIONABSTRACT_H
#define ACTIONABSTRACT_H

#include <memory>

class DeviceInterface;

class AbstractAction
{
    friend DeviceInterface;
    friend class AbstractResponse;

public:
    using actionHandle_t = std::unique_ptr<AbstractAction>;
    using uid_t = DeviceInterface*;

public:
    virtual ~AbstractAction() = default;

    uid_t requestor() const noexcept { return m_requestor; }

private:
    uid_t m_requestor;

};

#endif // ACTIONABSTRACT_H
