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
    _boolValues.clear();
    _vecValues.clear();
    _stringValues.clear();
    _objectValues.clear();
}

vector<FBlackboardKeyInfo> Blackboard::Get_AllKeys() const
{
    vector<FBlackboardKeyInfo> keys;

    for (const auto& [key, _] : _intValues)
        keys.push_back({ key, EBlackboardValueType::Int });

    for (const auto& [key, _] : _floatValues)
        keys.push_back({ key, EBlackboardValueType::Float });

    for (const auto& [key, _] : _boolValues)
        keys.push_back({ key, EBlackboardValueType::Bool });

    for (const auto& [key, _] : _vecValues)
        keys.push_back({ key, EBlackboardValueType::Vector3 });

    for (const auto& [key, _] : _stringValues)
        keys.push_back({ key, EBlackboardValueType::String });

    return keys;
}

void Blackboard::Set_ValueAsInt(const string& key, int32 value)
{
    _intValues[key] = value;
}

void Blackboard::Set_ValueAsFloat(const string& key, float value)
{
    _floatValues[key] = value;
}

void Blackboard::Set_ValueAsBool(const string& key, bool value)
{
    _boolValues[key] = value;
}

void Blackboard::Set_ValueAsVector(const string& key, Vec3 value)
{
    _vecValues[key] = value;
}

void Blackboard::Set_ValueAsString(const string& key, const string& value)
{
    _stringValues[key] = value;
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

bool Blackboard::Get_ValueAsBool(const string& key)
{
    if (_boolValues.contains(key))
        return _boolValues[key];

    LOG_WARN("Blackboard Key Not Found (Bool): {}", key);

    return false;
}

Vec3 Blackboard::Get_ValueAsVector(const string& key)
{
    if (_vecValues.contains(key))
        return _vecValues[key];

    LOG_WARN("Blackboard Key Not Found (Vector): {}", key);

    return Vec3::Zero;
}

string Blackboard::Get_ValueAsString(const string& key)
{
    auto iter = _stringValues.find(key);
    if (iter != _stringValues.end())
        return iter->second;

    LOG_WARN("Blackboard Key Not Found (String): {}", key);
    return "";
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
    if (_boolValues.contains(key))      return true;
    if (_stringValues.contains(key))    return true;

    return false;
}

json Blackboard::Serialize_ToJson() const
{
    json root;

    // Int
    json ints = json::object();
    for (const auto& [key, value] : _intValues)
        ints[key] = value;

    root["ints"] = ints;

    // FLoat
    json floats = json::object();
    for (const auto& [key, value] : _floatValues)
        floats[key] = value;

    root["floats"] = floats;

    // Bool
    json bools = json::object();

    for (const auto& [key, value] : _boolValues)
        bools[key] = value;

    root["bools"] = bools;

    // Vector
    json vectors = json::object();
    for (const auto& [key, value] : _vecValues)
    {
        vectors[key] =
        {
            {"x", value.x},
            {"y", value.y},
            {"z", value.z}
        };
    }
    root["vectors"] = vectors;

    // String
    json strings = json::object();
    for (const auto& [key, value] : _stringValues)
        strings[key] = value;
    
    root["strings"] = strings;

    // Object는 런타임 참조

    return root;
}

void Blackboard::Deserialize_FromJson(const json& data)
{
    Initialize(); // 기존 값 초기화

    if (data.contains("ints"))
    {
        for (auto& [key, value] : data["ints"].items())
            _intValues[key] = value.get<int32>();
    }

    if (data.contains("floats"))
    {
        for (auto& [key, value] : data["floats"].items())
            _floatValues[key] = value.get<float>();
    }

    if (data.contains("bools"))
    {
        for (auto& [key, value] : data["bools"].items())
            _boolValues[key] = value.get<bool>();
    }

    if (data.contains("vectors"))
    {
        for (auto& [key, value] : data["vectors"].items())
        {
            Vec3 vec;
            vec.x = value["x"].get<float>();
            vec.y = value["y"].get<float>();
            vec.z = value["z"].get<float>();
            _vecValues[key] = vec;
        }
    }

    if (data.contains("strings"))
    {
        for (auto& [key, value] : data["strings"].items())
            _stringValues[key] = value.get<string>();
    }
}

Shared<Blackboard> Blackboard::Create()
{
    auto instance = make_shared<Blackboard>();
    instance->Initialize();

    return instance;
}
