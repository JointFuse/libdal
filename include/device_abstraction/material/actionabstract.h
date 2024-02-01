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
    uid_t m_requestor{ nullptr };

};

class PriorityAction : public AbstractAction
{
public:
    enum ActionPriority{
        Last    ,
        First   ,
        Default ,
        Low     ,
        Middle  ,
        High    ,
    };

public:
    void setPriority(ActionPriority prior)  { m_priority = prior; }
    ActionPriority priority()               const noexcept { return m_priority; }

private:
    ActionPriority m_priority = Default;

};

#endif // ACTIONABSTRACT_H
