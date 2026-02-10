#pragma once

#include "BTNode.h"

NS_BEGIN(Engine)

class ENGINE_DLL BTComposite : public BTNode
{
public:
    explicit BTComposite();
    virtual ~BTComposite() = default;

public:
    void            Add_Child(Shared<BTNode> node);
    virtual void    OnTerminate(EBTNodeResult result) override;

protected:
    vector<Shared<BTNode>> _children;
    int32 _runningChildIndex = 0;

};

// ==========================================
//              Selector (OR)
// ==========================================
// 자식들 중 하나라도 성공한다면 성공(Succeeded) 반환, 실패하면 다음 자식 실행
class ENGINE_DLL BTSelector : public BTComposite
{
public:
    explicit BTSelector() = default;
    virtual ~BTSelector() = default;

public:
    EBTNodeResult Update(float timeDelta) override;

};

// ==========================================
//              Sequence (AND)
// ==========================================
// 자식들이 모두 성공해야 성공. 하나라도 실패하면 실패(Faield) 반환
class ENGINE_DLL BTSequence : public BTComposite
{
public:
    explicit BTSequence() = default;
    virtual ~BTSequence() = default;
    
public:
    EBTNodeResult Update(float timeDelta) override;

};


NS_END
