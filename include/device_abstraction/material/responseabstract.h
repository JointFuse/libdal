#ifndef RESPONSEABSTRACT_H
#define RESPONSEABSTRACT_H

#include <memory>

#include <QMetaType>

#include "actionabstract.h"

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

Q_DECLARE_METATYPE(AbstractResponse*)

#endif // RESPONSEABSTRACT_H
