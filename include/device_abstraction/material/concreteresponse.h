#ifndef CONCRETERESPONSE_H
#define CONCRETERESPONSE_H

#include <string>

#include "responseabstract.h"

struct DataBaseResponse : public AbstractResponse
{
//    std::vector<Types::DBValueMap> data;
//    int uid{ DBInterface::Invalid };
};

struct PTZResponse : public AbstractResponse
{
    bool success;
    std::string description;
//    std::map<AxisType, AxisData> device_state;
};

#endif // CONCRETERESPONSE_H
