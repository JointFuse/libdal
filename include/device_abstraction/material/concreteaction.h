#ifndef CONCRETEACTION_H
#define CONCRETEACTION_H

#include <memory>
#include <string>

#include "actionabstract.h"

struct DataBaseAction : public AbstractAction
{
    std::string query;
//    Types::DBValueMap bindable;
//    DBInterface* requestor{ nullptr };
//    int uid{ DBInterface::Invalid };
};

#endif // CONCRETEACTION_H
