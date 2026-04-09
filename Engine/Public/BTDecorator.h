#pragma once

#include "BTNode.h"

NS_BEGIN(Engine)

class ENGINE_DLL BTDecorator : public BTNode
{
public:
    explicit BTDecorator();
    explicit BTDecorator(const BTDecorator& rhs);
    virtual ~BTDecorator() = default;

public:
    virtual void    Initialize() override;

    virtual void    Set_Blackboard(Shared<Blackboard> blackboard) override;
    virtual void    Set_Owner(Shared<GameObject> owner) override;
    virtual void    OnTerminate(EBTNodeResult result) override;
    virtual void    Gather_NodeResults(map<int, EBTNodeResult>& outResults) override;

public:
    void            Set_Child(Shared<BTNode> child) { _child = child; }
    Shared<BTNode>  Get_Child() const { return _child; }

protected:
    Shared<BTNode> _child;

};

NS_END
