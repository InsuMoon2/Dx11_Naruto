#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Model;
class GameObject;
class AnimNotify;
class AnimNotifyState;

struct FAnimNotifyContext
{
    GameObject* owner = nullptr;
    Model*  model = nullptr;

    string  modelGuid;
    string  clipName;

    float   previousTimeSec = 0.f;
    float   currentTimeSec = 0.f;
    float   deltaTime = 0.f;

    bool    isLooping = false;
    bool    wrapped = false;
    bool    isPreview = false;
};

struct FAnimNotifyEventEntry
{
    float timeSec = 0.f;
    Shared<AnimNotify> notify;
};

struct FAnimNotifyStateEntry
{
    float startSec = 0.f;
    float durationSec = 0.f;
    Shared<AnimNotifyState> notifyState;
};

struct FAnimNotifyClipData
{
    string clipName;
    int32 displayFps = 30;

    vector<FAnimNotifyEventEntry> notifies;
    vector<FAnimNotifyStateEntry> notifyStates;
};

struct FAnimNotifyAsset
{
    string modelGuid;

    vector<FAnimNotifyClipData> clips;
};

struct FActiveAnimNotifyState
{
    string clipName;         

    int32 stateIndex = -1;   
    float startSec = 0.f;    
    float endSec = 0.f;      

    float lastTimeSec = 0.f; 
                             

    Shared<AnimNotifyState> notifyState;
};


NS_END
