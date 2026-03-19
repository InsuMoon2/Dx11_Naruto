#include "pch.h"
#include "AnimNotify_Serializer.h"

#include <fstream>

#include "AnimNotify_Factory.h"
#include "AnimNotify.h"
#include "AnimNotifyState.h"

json AnimNotify_Serializer::To_Json(const FAnimNotifyAsset& asset)
{
    json root;
    root["model_guid"] = asset.modelGuid;
    root["clips"] = json::array();

    for (const auto& clip : asset.clips)
    {
        json clipJson;
        clipJson["clip_name"] = clip.clipName;
        clipJson["display_fps"] = clip.displayFps;
        clipJson["notifies"] = json::array();
        clipJson["notify_states"] = json::array();

        for (const auto& entry : clip.notifies)
        {
            if (!entry.notify)
                continue;

            clipJson["notifies"].push_back({
                { "time_sec", entry.timeSec },
                { "type", entry.notify->Get_TypeName() },
                { "payload", entry.notify->Serialize_Payload() }
                });
        }

        for (const auto& entry : clip.notifyStates)
        {
            if (!entry.notifyState)
                continue;

            clipJson["notify_states"].push_back({
                { "start_sec", entry.startSec },
                { "duration_sec", entry.durationSec },
                { "type", entry.notifyState->Get_TypeName() },
                { "payload", entry.notifyState->Serialize_Payload() }
                });
        }

        root["clips"].push_back(clipJson);
    }

    return root;
}

bool AnimNotify_Serializer::From_Json(const json& root, FAnimNotifyAsset& outAsset)
{
    outAsset = {};

    outAsset.modelGuid = root.value("model_guid", "");
    if (!root.contains("clips") || !root["clips"].is_array())
        return true;

    for (const auto& clipJson : root["clips"])
    {
        FAnimNotifyClipData clip;
        clip.clipName = clipJson.value("clip_name", "");
        clip.displayFps = max(1, clipJson.value("display_fps", 30));

        if (clipJson.contains("notifies") && clipJson["notifies"].is_array())
        {
            for (const auto& notifyJson : clipJson["notifies"])
            {
                const string typeName = notifyJson.value("type", "");
                auto instance = AnimNotify_Factory::Create_Notify(typeName);
                if (!instance)
                {
                    LOG_WARN("Unknown AnimNotify type: {}", typeName);
                    continue;
                }

                instance->Deserialize_Payload(notifyJson.value("payload", json::object()));

                FAnimNotifyEventEntry entry;
                entry.timeSec = notifyJson.value("time_sec", 0.f);
                entry.notify = instance;
                clip.notifies.push_back(entry);
            }
        }

        if (clipJson.contains("notify_states") && clipJson["notify_states"].is_array())
        {
            for (const auto& stateJson : clipJson["notify_states"])
            {
                const string typeName = stateJson.value("type", "");
                auto instance = AnimNotify_Factory::Create_NotifyState(typeName);
                if (!instance)
                {
                    LOG_WARN("Unknown AnimNotifyState type: {}", typeName);
                    continue;
                }

                instance->Deserialize_Payload(stateJson.value("payload", json::object()));

                FAnimNotifyStateEntry entry;
                entry.startSec = stateJson.value("start_sec", 0.f);
                entry.durationSec = max(0.f, stateJson.value("duration_sec", 0.f));
                entry.notifyState = instance;
                clip.notifyStates.push_back(entry);
            }
        }

        outAsset.clips.push_back(clip);
    }

    return true;
}

bool AnimNotify_Serializer::Save_ToFile(const wstring& filePath, const FAnimNotifyAsset& asset)
{
    fs::create_directories(fs::path(filePath).parent_path());

    ofstream file(filePath);
    if (!file.is_open())
        return false;

    file << To_Json(asset).dump(4);

    return true;
}

bool AnimNotify_Serializer::Load_FromFile(const wstring& filePath, FAnimNotifyAsset& outAsset)
{
    if (!fs::exists(filePath))
    {
        outAsset = {};
        return false;
    }

    ifstream file(filePath);
    if (!file.is_open())
        return false;

    json root;
    file >> root;
    return From_Json(root, outAsset);
}

fs::path AnimNotify_Serializer::Get_BaseFolderPath()
{
    return fs::path("../../Client/Bin/Resources/Data/json/AnimNotifies");
}

fs::path AnimNotify_Serializer::Get_ModelNotifyFilePath(const string& modelGuid)
{
    return Get_BaseFolderPath() / (modelGuid + ".animnotify.json");
}

FAnimNotifyClipData* AnimNotify_Serializer::Find_Clip(FAnimNotifyAsset& asset, const string& clipName)
{
    for (auto& clip : asset.clips)
    {
        if (clip.clipName == clipName)
            return &clip;
    }

    return nullptr;
}

const FAnimNotifyClipData* AnimNotify_Serializer::Find_Clip(const FAnimNotifyAsset& asset, const string& clipName)
{
    for (const auto& clip : asset.clips)
    {
        if (clip.clipName == clipName)
            return &clip;
    }

    return nullptr;
}

FAnimNotifyClipData& AnimNotify_Serializer::Get_OrAddClip(FAnimNotifyAsset& asset, const string& clipName)
{
    if (auto* clip = Find_Clip(asset, clipName))
        return *clip;

    FAnimNotifyClipData newClip;

    newClip.clipName = clipName;
    asset.clips.push_back(newClip);

    return asset.clips.back();
}
