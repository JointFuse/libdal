#ifndef PROMISE_H
#define PROMISE_H

#include <future>

#include "actionabstract.h"
#include "responseabstract.h"

namespace dal { ////////////////////////////////////////////////////////////////
/**
 * @brief The PromiseAction class
 */
class PromiseAction : public PriorityAction
{
public:
    std::promise<AbstractResponse::responseHandle_t> promise;

};
/**
 * @brief The PromiseResponse class
 */
class PromiseResponse : public AbstractResponse
{
public:
    PromiseResponse(PromiseAction* act)
        : AbstractResponse{ act->requestor() }
        , promise { std::move(act->promise) } {

    }

public:
    std::promise<AbstractResponse::responseHandle_t> promise;

};

} //////////////////////////////////////////////////////////////////////////////

#endif // PROMISE_H
