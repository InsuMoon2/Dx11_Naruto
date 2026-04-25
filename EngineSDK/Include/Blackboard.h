#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class GameObject;

class ENGINE_DLL Blackboard : public Base
{
public:
    explicit Blackboard();
    virtual ~Blackboard() = default;

public:
    // 메모리 풀링이나 복제 등을 위한 초기화
    virtual void Initialize();

    vector<FBlackboardKeyInfo> Get_AllKeys() const;

    // 에디터에서 선택한 Blackboard 키를 삭제할 때 호출한다.
    // 같은 이름이 여러 타입 맵에 남아 있어도 한 번에 정리한다.
    bool Remove_Key(const string& key);

public:
    // 데이터 설정
    void Set_ValueAsInt(const string& key, int32 value);
    void Set_ValueAsFloat(const string& key, float value);
    void Set_ValueAsBool(const string& key, bool value);
    void Set_ValueAsVector(const string& key, Vec3 value);
    void Set_ValueAsString(const string& key, const string& value);
    void Set_ValueAsObject(const string& key, Shared<GameObject> value);

    // 데이터 가져오기
    int32 Get_ValueAsInt(const string& key);
    float Get_ValueAsFloat(const string& key);
    bool  Get_ValueAsBool(const string& key);
    Vec3  Get_ValueAsVector(const string& key);
    string Get_ValueAsString(const string& key);
    Shared<GameObject> Get_ValueAsObject(const string& key);

    bool HasKey(const string& key) const;

    // 직렬화 (에디터용)
    json Serialize_ToJson() const;
    void Deserialize_FromJson(const json& data);

private:
    map<string, int32>              _intValues;
    map<string, float>              _floatValues;
    map<string, bool>               _boolValues;
    map<string, Vec3>               _vecValues;
    map<string, string>             _stringValues;
    map<string, Shared<GameObject>> _objectValues;

public:
    static Shared<Blackboard> Create();
};

NS_END
