#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class GameObject;

struct FEvent
{
    EEventType  eventType;
    virtual ~FEvent() = default;
};

struct FEvent_Object : public FEvent
{
public:
    FEvent_Object(EEventType type, shared_ptr<GameObject> obj)
    {
        eventType = type;
        targetObject = obj;
    }

public:
    shared_ptr<GameObject> targetObject;

    static shared_ptr<FEvent_Object> Create(EEventType type, shared_ptr<GameObject> obj)
    {
        return make_shared<FEvent_Object>(type, obj);
    }
};

struct FEvent_Level : public FEvent
{
public:
    FEvent_Level(EEventType type, uint32 index)
    {
        eventType = type;
        levelIndex = index;
    }

public:
    uint32 levelIndex;

    static shared_ptr<FEvent_Level> Create(EEventType type, uint32 levelIndex)
    {
        return make_shared<FEvent_Level>(type, levelIndex);
    }
};

using EventCallback = function<void(shared_ptr<FEvent>)>;

class ENGINE_DLL Event_Manager : public Base
{
    DECLARE_SINGLETON(Event_Manager)

public:
    explicit Event_Manager() = default;
    virtual ~Event_Manager() = default;

public:
    void Subscribe(EEventType type, EventCallback callback); // 구독
    void Publish(shared_ptr<FEvent> event);                  // 발행
    void ProcessEvents();                                    // 실행

private:
    queue<shared_ptr<FEvent>> _eventQueue;
    umap<EEventType, vector<EventCallback>> _subscribers;

};

NS_END
