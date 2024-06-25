#ifndef ACTIONABSTRACT_H
#define ACTIONABSTRACT_H

#include <memory>
#include <functional>

namespace dal { ////////////////////////////////////////////////////////////////

class DeviceInterface;
/**
 * @brief The AbstractAction class
 */
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
/**
 * @brief The PriorityAction class
 */
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
/**
 * @brief The ReadyAction class
 */
class ReadyAction : public AbstractAction
{
public:
    ReadyAction(std::function<bool(void)> pred)
        : m_predicate{ pred }
    {}

    bool isReady() const { return m_predicate(); }

private:
    std::function<bool(void)> m_predicate;

};

} //////////////////////////////////////////////////////////////////////////////

#endif // ACTIONABSTRACT_H
