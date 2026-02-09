#include "pch.h"
#include "Blackboard.h"
#include "GameObject.h"

Blackboard::Blackboard()
{
}

void Blackboard::Initialize()
{
    _intValues.clear();
    _floatValues.clear();
    _vecValues.clear();
    _objectValues.clear();
}

void Blackboard::Set_ValueAsInt(const string& key, int32 value)
{
    _intValues[key] = value;
}

void Blackboard::Set_ValueAsFloat(const string& key, float value)
{
    _floatValues[key] = value;
}

void Blackboard::Set_ValueAsVector(const string& key, Vec3 value)
{
    _vecValues[key] = value;
}

void Blackboard::Set_ValueAsObject(const string& key, Shared<GameObject> value)
{
    // 이거 순환참조 조심해야함
    _objectValues[key] = value;
}

int32 Blackboard::Get_ValueAsInt(const string& key)
{
    if (_intValues.contains(key))
        return _intValues[key];

    LOG_WARN("Blackboard Key Not Found (Int): {}", key);
    return 0;
}

float Blackboard::Get_ValueAsFloat(const string& key)
{
    if (_floatValues.contains(key))
        return _floatValues[key];

    LOG_WARN("Blackboard Key Not Found (Float): {}", key);

    return 0.0f;
}

Vec3 Blackboard::Get_ValueAsVector(const string& key)
{
    if (_vecValues.contains(key))
        return _vecValues[key];

    LOG_WARN("Blackboard Key Not Found (Vector): {}", key);

    return Vec3::Zero;
}

Shared<GameObject> Blackboard::Get_ValueAsObject(const string& key)
{
    if (_objectValues.contains(key))
        return _objectValues[key];

    return nullptr;
}

bool Blackboard::HasKey(const string& key) const
{
    if (_intValues.contains(key))       return true;
    if (_floatValues.contains(key))     return true;
    if (_vecValues.contains(key))       return true;
    if (_objectValues.contains(key))    return true;

    return false;
}

Shared<Blackboard> Blackboard::Create()
{
    auto instance = make_shared<Blackboard>();
    instance->Initialize();

    return instance;
}
