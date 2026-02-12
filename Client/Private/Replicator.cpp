#include "pch.h"
#include "Replicator.h"
#include "CombatStat.h"
#include "GameObject.h"
#include "Transform.h"

#ifdef GetMessage
#undef GetMessage
#endif

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

Replicator::Replicator(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

Replicator::Replicator(const Replicator& rhs)
    : Component(rhs)
    , _cachedInfo(rhs._cachedInfo)
{
}

Replicator::~Replicator()
{
}

HRESULT Replicator::Initialize_Prototype()
{

    return S_OK;
}

HRESULT Replicator::Initialize(void* arg)
{
    CHECK_NULL(arg, E_FAIL);

    Protocol::ObjectInfo* info = static_cast<Protocol::ObjectInfo*>(arg);
    Sync_FromProtobuf(*info);

    return S_OK;
}

void Replicator::Sync_FromProtobuf(const Protocol::ObjectInfo& info)
{
    _cachedInfo = info;

    auto owner = Get_Owner();
    CHECK_NULL(owner);

    auto transform = owner->Get_Component<Transform>();

    if (transform)
    {
        // TODO : Position 업데이트 어떻게 할지?
    }

    if (info.has_stat())
    {
        auto combatStat = owner->Get_Component<CombatStat>();
        CHECK_NULL(combatStat);

        auto replicable = dynamic_pointer_cast<IReplicable>(combatStat);
        CHECK_NULL(replicable);

        Protocol::CombatStat stat = info.stat();
        replicable->Sync_FromProtobuf(stat);
    }

}

Protocol::ObjectInfo Replicator::ToProtobuf()
{
    // 클라->서버
    Protocol::ObjectInfo info = _cachedInfo;

    auto owner = Get_Owner();
    CHECK_NULL(owner, Protocol::ObjectInfo {});

    auto transform = owner->Get_Component<Transform>();

    if (transform)
    {
        // TODO : Position 업데이트 어떻게 할지?
    }

    auto combatStat = owner->Get_Component<CombatStat>();

    if (auto replicable = dynamic_pointer_cast<IReplicable>(combatStat))
    {
        Protocol::CombatStat stat;
        replicable->Serialize_ToProtobuf(stat);
        *info.mutable_stat() = stat;
    }

    return info;
}

json Replicator::To_Json() const
{
    json j = Component::To_Json(); // 부모에서는 type만

    j["componentType"] = "Replicator";
    j["objectId"] = _cachedInfo.objectid();

    if (_cachedInfo.has_stat())
    {
        j["stat"] = Protobuf_ToJson(_cachedInfo.stat());
    }

    return j;
}

void Replicator::From_Json(const json& data)
{
    Component::From_Json(data);

    if (data.contains("objectId"))
        _cachedInfo.set_objectid(data["objectId"]);

    if (data.contains("stat"))
    {
        auto stat = _cachedInfo.mutable_stat();
        Json_ToProtobuf(data["stat"], *stat);
    }
}

json Replicator::Protobuf_ToJson(const Message& message)
{
    json j;

    const auto* descriptor = message.GetDescriptor();
    const auto* reflection = message.GetReflection();

    for (int i = 0; i < descriptor->field_count(); i++)
    {
        const auto* field = descriptor->field(i);

        // 필드가 설정되지 않았으면 스킵
        if (!reflection->HasField(message, field))
            continue;

        string fieldName = field->name();

        switch (field->type())
        {
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
            j[fieldName] = reflection->GetFloat(message, field);
            break;

        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
            j[fieldName] = reflection->GetDouble(message, field);
            break;

        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
            j[fieldName] = reflection->GetInt32(message, field);
            break;

        case google::protobuf::FieldDescriptor::TYPE_INT64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
            j[fieldName] = reflection->GetInt64(message, field);
            break;

        case google::protobuf::FieldDescriptor::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
            j[fieldName] = reflection->GetUInt32(message, field);
            break;

        case google::protobuf::FieldDescriptor::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
            j[fieldName] = reflection->GetUInt64(message, field);
            break;

        case google::protobuf::FieldDescriptor::TYPE_BOOL:
            j[fieldName] = reflection->GetBool(message, field);
            break;

        case google::protobuf::FieldDescriptor::TYPE_STRING:
            j[fieldName] = reflection->GetString(message, field);
            break;

        case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
            // 중첩 메시지는 재귀 호출
            const auto& submsg = reflection->GetMessage(message, field);
            j[fieldName] = Protobuf_ToJson(submsg);
            break;
        }
    }

    return j;
}

void Replicator::Json_ToProtobuf(const json& j, Message& message)
{
    const auto* descriptor = message.GetDescriptor();
    const auto* reflection = message.GetReflection();

    for (auto& [key, value] : j.items())
    {
        const auto* field = descriptor->FindFieldByName(key);
        if (!field)
            continue;

        switch (field->type())
        {
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
            reflection->SetFloat(&message, field, value.get<float>());
            break;

        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
            reflection->SetDouble(&message, field, value.get<double>());
            break;

        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
            reflection->SetInt32(&message, field, value.get<int32>());
            break;

        case google::protobuf::FieldDescriptor::TYPE_INT64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
            reflection->SetInt64(&message, field, value.get<int64>());
            break;

        case google::protobuf::FieldDescriptor::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
            reflection->SetUInt32(&message, field, value.get<uint32>());
            break;

        case google::protobuf::FieldDescriptor::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
            reflection->SetUInt64(&message, field, value.get<uint64>());
            break;

        case google::protobuf::FieldDescriptor::TYPE_BOOL:
            reflection->SetBool(&message, field, value.get<bool>());
            break;

        case google::protobuf::FieldDescriptor::TYPE_STRING:
            reflection->SetString(&message, field, value.get<std::string>());
            break;

        case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
            auto* submsg = reflection->MutableMessage(&message, field);
            Json_ToProtobuf(value, *submsg);
            break;
        }

    }
}

shared_ptr<Replicator> Replicator::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Replicator>(device, context);
    instance->Initialize_Prototype();

    return instance;
}

shared_ptr<Component> Replicator::Clone(void* arg)
{
    auto instance = make_shared<Replicator>(*this);
    instance->Initialize(arg);

    return instance;
}

void Replicator::Free()
{
    Component::Free();
}
