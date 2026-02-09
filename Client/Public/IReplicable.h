#pragma once

NS_BEGIN(Client)

class IReplicable
{
public:
    IReplicable() = default;
    virtual ~IReplicable() = default;

public:
    // Protobuf 메시지에서 데이터 로드
    virtual void Sync_FromProtobuf(Message& message) = 0;

    // Protobuf 메시지로 데이터 저장
    virtual void Serialize_ToProtobuf(Message& message) const = 0;
};

NS_END
