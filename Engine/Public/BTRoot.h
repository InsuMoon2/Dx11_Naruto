#pragma once

#include "BTComposite.h"

NS_BEGIN(Engine)

class BTRoot : public BTComposite
{
public:
    explicit BTRoot() = default;
    virtual ~BTRoot() = default;

public:
    virtual EBTNodeResult  Update(float timeDelta) override;
    virtual Shared<BTNode> Clone() override;
};

NS_END
