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

public:
    static Shared<BTRoot> Create();
    virtual Shared<BTNode> Clone() override;
};

NS_END
