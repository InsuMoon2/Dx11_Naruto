#include "pch.h"
#include "Property_Command.h"

Property_Command::Property_Command(void* memberPtr, EPropertyType type, const json& oldValue, const json& newValue)
    : _memberPtr(memberPtr), _type(type)
    , _oldValue(oldValue), _newValue(newValue)
{

}

void Property_Command::Execute()
{

}

void Property_Command::Undo()
{
    Apply(_oldValue);
}

void Property_Command::Redo()
{
    Apply(_newValue);
}

string Property_Command::Get_Description() const
{
    return "Property Change";
}

void Property_Command::Apply(const json& val)
{
    switch (_type)
    {
    case EPropertyType::Float:
        *static_cast<float*>(_memberPtr) = val.get<float>();
        break;

    case EPropertyType::Int:
    case EPropertyType::Enum:
        *static_cast<int*>(_memberPtr) = val.get<int>();
        break;

    case EPropertyType::Bool:
        *static_cast<bool*>(_memberPtr) = val.get<bool>();
        break;

    case EPropertyType::String:
        *static_cast<string*>(_memberPtr) = val.get<string>();
        break;

    case EPropertyType::Vec3:
    {
        float* v = static_cast<float*>(_memberPtr);
        v[0] = val[0];
        v[1] = val[1];
        v[2] = val[2];
        break;
    }
    case EPropertyType::Color:
    {
        float* v = static_cast<float*>(_memberPtr);
        v[0] = val[0];
        v[1] = val[1];
        v[2] = val[2];
        v[3] = val[3];
        break;
    }
    default: 
        break;
    }
}

Shared<Property_Command> Property_Command::Create(void* memberPtr, EPropertyType type, const json& oldValue,
                                                  const json& newValue)
{
    return make_shared<Property_Command>(memberPtr, type, oldValue, newValue);
}

void Property_Command::Free()
{
    ICommand::Free();
}
