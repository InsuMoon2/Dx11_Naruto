#pragma once

#include "ICommand.h"

NS_BEGIN(Editor)

class Property_Command : public ICommand
{
public:
    explicit Property_Command(void* memberPtr, EPropertyType type,
                                const json& oldValue, const json& newValue);

    virtual ~Property_Command() = default;

public:
    virtual void Execute() override;
    virtual void Undo()    override;
    virtual void Redo()    override;

    virtual string Get_Description() const override;

private:
    void Apply(const json& val);

private:
    void*           _memberPtr = nullptr;
    EPropertyType   _type;
    json            _oldValue;
    json            _newValue;

public:
    static Shared<Property_Command> Create(void* memberPtr, EPropertyType type,
                                const json& oldValue, const json& newValue);

    void Free() override;
};

NS_END
