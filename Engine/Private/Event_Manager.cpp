#include "pch.h"
#include "Event_Manager.h"

IMPLEMENT_SINGLETON(Event_Manager)

void Event_Manager::Subscribe(EEventType type, EventCallback callback)
{
    _subscribers[type].emplace_back(callback);
}

void Event_Manager::Publish(shared_ptr<FEvent> event)
{
    _eventQueue.emplace(event);
}

void Event_Manager::ProcessEvents()
{
    while (!_eventQueue.empty())
    {
        auto event = _eventQueue.front();

        _eventQueue.pop();

        if (_subscribers.contains(event->eventType))
        {
            for (auto& callback : _subscribers[event->eventType])
            {
                callback(event);
            }
        }
    }
}
