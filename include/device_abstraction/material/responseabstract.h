#ifndef RESPONSEABSTRACT_H
#define RESPONSEABSTRACT_H

#include <memory>

#include "actionabstract.h"

namespace dal { ////////////////////////////////////////////////////////////////
/**
 * @brief The AbstractResponse class
 */
class AbstractResponse : public AbstractAction
{
public:
    using responseHandle_t = std::unique_ptr<AbstractResponse>;

private:
    using AbstractAction::actionHandle_t;

public:
    AbstractResponse() = delete;

    AbstractResponse(uid_t cli) { m_requestor = cli; }

};

} //////////////////////////////////////////////////////////////////////////////

#endif // RESPONSEABSTRACT_H
