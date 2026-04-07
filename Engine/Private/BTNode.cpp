#include "pch.h"
#include "BtNode.h"

BTNode::BTNode()
{

}

BTNode::BTNode(const BTNode& rhs)
    : Base(rhs)
    , _lastResult(EBTNodeResult::NotExecuted)
    , _debugId(rhs._debugId)
{

}

BTNode::~BTNode()
{
}

void BTNode::Initialize()
{
    _lastResult = EBTNodeResult::NotExecuted;
}

EBTNodeResult BTNode::Update(float timeDelta)
{

    return EBTNodeResult::Succeeded;
}

static string Resolve_BTNodePropertyJsonKey(const Engine::FPropertyInfo& prop)
{
    // JSON 키가 지정돼 있으면 그 값을 우선 사용
    if (!prop.jsonKey.empty())
        return prop.jsonKey;

    // 별도 JSON 키가 없으면 표시 이름을 그대로 키로 사용
    return prop.name;
}

json BTNode::Serialize_ToJson()
{
    json root = json::object();

    auto& info = GetReflectionInfo();
    const char* basePtr = reinterpret_cast<const char*>(this);

    for (const auto& prop : info.properties)
    {
        const string key = Resolve_BTNodePropertyJsonKey(prop);
        const void* memberPtr = basePtr + prop.offset;

        switch (prop.type)
        {
        case EPropertyType::Float:
        case EPropertyType::ReadOnly:
            root[key] = *static_cast<const float*>(memberPtr);
            break;

        case EPropertyType::Int:
        case EPropertyType::Enum:
            root[key] = *static_cast<const int*>(memberPtr);
            break;

        case EPropertyType::Bool:
            root[key] = *static_cast<const bool*>(memberPtr);
            break;

        case EPropertyType::Vec2:
        {
            const float* v = static_cast<const float*>(memberPtr);
            root[key] = { v[0], v[1] };
            break;
        }

        case EPropertyType::Vec3:
        {
            const float* v = static_cast<const float*>(memberPtr);
            root[key] = { v[0], v[1], v[2] };
            break;
        }

        case EPropertyType::Vec4:
        case EPropertyType::Color:
        {
            const float* v = static_cast<const float*>(memberPtr);
            root[key] = { v[0], v[1], v[2], v[3] };
            break;
        }

        case EPropertyType::String:
            root[key] = *static_cast<const string*>(memberPtr);
            break;

        default:
            break;
        }
    }

    return root;
}

void BTNode::Deserialize_FromJson(const json& data)
{
    auto& info = GetReflectionInfo();
    char* basePtr = reinterpret_cast<char*>(this);

    for (const auto& prop : info.properties)
    {
        const string key = Resolve_BTNodePropertyJsonKey(prop);
        if (!data.contains(key))
            continue;

        void* memberPtr = basePtr + prop.offset;

        switch (prop.type)
        {
        case EPropertyType::Float:
        case EPropertyType::ReadOnly:
            *static_cast<float*>(memberPtr) = data[key].get<float>();
            break;

        case EPropertyType::Int:
        case EPropertyType::Enum:
            *static_cast<int*>(memberPtr) = data[key].get<int>();
            break;

        case EPropertyType::Bool:
            *static_cast<bool*>(memberPtr) = data[key].get<bool>();
            break;

        case EPropertyType::Vec2:
        {
            auto arr = data[key];
            float* v = static_cast<float*>(memberPtr);
            v[0] = arr[0];
            v[1] = arr[1];
            break;
        }

        case EPropertyType::Vec3:
        {
            auto arr = data[key];
            float* v = static_cast<float*>(memberPtr);
            v[0] = arr[0];
            v[1] = arr[1];
            v[2] = arr[2];
            break;
        }

        case EPropertyType::Vec4:
        case EPropertyType::Color:
        {
            auto arr = data[key];
            float* v = static_cast<float*>(memberPtr);
            v[0] = arr[0];
            v[1] = arr[1];
            v[2] = arr[2];
            v[3] = arr[3];
            break;
        }

        case EPropertyType::String:
            *static_cast<string*>(memberPtr) = data[key].get<string>();
            break;

        default:
            break;
        }
    }
}

void BTNode::Gather_NodeResults(map<int, EBTNodeResult>& outResults)
{
    if (_debugId != -1)
        outResults[_debugId] = _lastResult;
}
