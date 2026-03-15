#include "pch.h"
#include "UI_AnimSerializer.h"
#include <fstream>
#include <magic_enum/magic_enum.hpp>

string UI_AnimSerializer::Property_ToString(EUIAnimProperty property)
{
    if (property == EUIAnimProperty::END)
        return "";

    return string(magic_enum::enum_name(property));
}

bool UI_AnimSerializer::String_ToProperty(const string& text, EUIAnimProperty& outProperty)
{
    auto result = magic_enum::enum_cast<EUIAnimProperty>(text);

    if (!result.has_value())
        return false;

    if (result.value() == EUIAnimProperty::END)
        return false;

    outProperty = result.value();

    return true;
}

json UI_AnimSerializer::Key_ToJson(const FUIAnimKey& key)
{
    json j;

    j["frame"] = key.frame;
    j["value"] = { key.value.x, key.value.y, key.value.z, key.value.w };

    return j;
}

bool UI_AnimSerializer::Json_ToKey(const json& j, FUIAnimKey& outKey)
{
    if (!j.is_object()) return false;
    if (!j.contains("frame") || !j["frame"].is_number_integer()) return false;
    if (!j.contains("value") || !j["value"].is_array() || j["value"].size() < 4) return false;

    outKey.frame = j["frame"].get<int32>();
    outKey.value = Vec4(
        j["value"][0].get<float>(),
        j["value"][1].get<float>(),
        j["value"][2].get<float>(),
        j["value"][3].get<float>());

    return true;
}

json UI_AnimSerializer::Track_ToJson(const FUIAnimTrack& track)
{
    json j;

    j["property"] = Property_ToString(track.property);
    j["keys"] = json::array();

    for (const auto& key : track.keys)
        j["keys"].push_back(Key_ToJson(key));

    return j;
}

bool UI_AnimSerializer::Json_ToTrack(const json& j, FUIAnimTrack& outTrack)
{
    if (!j.is_object()) return false;
    if (!j.contains("property") || !j["property"].is_string()) return false;

    EUIAnimProperty property = EUIAnimProperty::PositionX;
    if (!String_ToProperty(j["property"].get<string>(), property))
    {
        return false;
    }

    outTrack.property = property;
    outTrack.keys.clear();

    if (j.contains("keys") && j["keys"].is_array())
    {
        for (const auto& keyJson : j["keys"])
        {
            FUIAnimKey key{};

            if (Json_ToKey(keyJson, key))
            {
                outTrack.keys.push_back(key);
            }
        }
    }

    Normalize_Track(outTrack);

    return true;
}

void UI_AnimSerializer::Normalize_Asset(FUIAnimAsset& asset)
{
    if (asset.fps <= 0) asset.fps = 60;
    if (asset.startFrame < 0) asset.startFrame = 0;
    if (asset.endFrame < asset.startFrame) std::swap(asset.startFrame, asset.endFrame);

    for (auto& track : asset.tracks)
    {
        Normalize_Track(track);
    }

    bool used[static_cast<int32>(EUIAnimProperty::END)] = {};
    vector<FUIAnimTrack> uniqueTracks;

    for (const auto& track : asset.tracks)
    {
        int32 index = static_cast<int32>(track.property);
        if (index < 0 || index >= static_cast<int32>(EUIAnimProperty::END))
            continue;
        if (used[index])
            continue;

        used[index] = true;
        uniqueTracks.push_back(track);
    }

    asset.tracks.swap(uniqueTracks);
}

void UI_AnimSerializer::Normalize_Track(FUIAnimTrack& track)
{
    sort(track.keys.begin(), track.keys.end(),
        [](const FUIAnimKey& a, const FUIAnimKey& b)
        {
            return a.frame < b.frame;
        });

    vector<FUIAnimKey> uniqueKeys;
    for (auto key : track.keys)
    {
        if (key.frame < 0)
            key.frame = 0;

        if (!uniqueKeys.empty() && uniqueKeys.back().frame == key.frame)
            uniqueKeys.back() = key;
        else
            uniqueKeys.push_back(key);
    }

    track.keys.swap(uniqueKeys);
}

json UI_AnimSerializer::To_Json(const FUIAnimAsset& asset)
{
    FUIAnimAsset normalized = asset;
    Normalize_Asset(normalized);

    json root;
    root["name"] = normalized.name;
    root["targetName"] = Utils::ToString(normalized.targetName);
    root["fps"] = normalized.fps;
    root["startFrame"] = normalized.startFrame;
    root["endFrame"] = normalized.endFrame;
    root["loop"] = normalized.loop;
    root["tracks"] = json::array();

    for (const auto& track : normalized.tracks)
        root["tracks"].push_back(Track_ToJson(track));

    return root;
}

bool UI_AnimSerializer::From_Json(const json& root, FUIAnimAsset& outAsset)
{
    if (!root.is_object())
        return false;

    outAsset = {};
    outAsset.name = root.value("name", string(""));
    outAsset.targetName = Utils::ToWString(root.value("targetName", string("")));
    outAsset.fps = root.value("fps", 60);
    outAsset.startFrame = root.value("startFrame", 0);
    outAsset.endFrame = root.value("endFrame", 60);
    outAsset.loop = root.value("loop", false);

    if (root.contains("tracks") && root["tracks"].is_array())
    {
        for (const auto& trackJson : root["tracks"])
        {
            FUIAnimTrack track{};
            if (Json_ToTrack(trackJson, track))
                outAsset.tracks.push_back(track);
        }
    }

    Normalize_Asset(outAsset);
    return true;
}

Shared<FUIAnimAsset> UI_AnimSerializer::Load_FromFile(const wstring& fullPath)
{
    ifstream file(fullPath);
    if (!file.is_open())
        return nullptr;

    json root;
    try { file >> root; }
    catch (...) { return nullptr; }

    auto asset = make_shared<FUIAnimAsset>();
    if (!From_Json(root, *asset))
        return nullptr;

    return asset;
}

bool UI_AnimSerializer::Save_ToFile(const wstring& fullPath, const FUIAnimAsset& asset)
{
    try
    {
        fs::path path(fullPath);
        if (!fs::exists(path.parent_path()))
            fs::create_directories(path.parent_path());

        ofstream file(fullPath);
        if (!file.is_open())
            return false;

        file << To_Json(asset).dump(4);
        return true;
    }
    catch (...)
    {
        return false;
    }
}
