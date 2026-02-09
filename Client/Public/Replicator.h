#pragma once

#include "Component.h"
#include "IReplicable.h"

NS_BEGIN(Client)

class Replicator : public Component
{
    GENERATED_COMPONENT(Replicator, Protocol::COMPONENT_TYPE_REPLICATOR)

public:
    explicit Replicator(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Replicator(const Replicator& rhs);
    virtual ~Replicator();

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

public:
    uint64  Get_ObjectID() const { return _cachedInfo.objectid(); }
    void    Set_ObjectID(uint64 id) { _cachedInfo.set_objectid(id); }

public: /* Protobuf 동기화 */
    // 서버 → 클라이언트: 받은 데이터를 GameObject 컴포넌트들에 분배
    void    Sync_FromProtobuf(const Protocol::ObjectInfo& info);

    // 클라이언트 → 서버: GameObject의 최신 데이터를 Protobuf로 변환
    Protocol::ObjectInfo ToProtobuf();

public: /* 파싱 직렬화 (에디터) */
    json To_Json() const override;
    void From_Json(const json& data) override;

private: /* Protobuf <-> Json 헬퍼용 함수 */
    static json Protobuf_ToJson(const Message& message);
    static void Json_ToProtobuf(const json& j, Message& message);

private:
    Protocol::ObjectInfo _cachedInfo;   // 서버에서 받은 원본 캐싱
    bool _isDirty = false;              // 로컬 변경 여부

public:
    static shared_ptr<Replicator> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    shared_ptr<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
