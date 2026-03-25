#include "pch.h"
#include "CameraTrack_Serializer.h"
#include "Utils.h"
#include <fstream>

json CameraTrack_Serializer::To_Json(const FCameraSequenceAsset& asset)
{
    json root;
    root["name"] = asset.name;

    // 트랙
    json trackJson;
    trackJson["track_name"] = asset.track.trackName;
    trackJson["fps"] = asset.track.fps;
    trackJson["total_frames"] = asset.track.totalFrame;
    trackJson["keys"] = json::array();

    for (const auto& key : asset.track.keys)
    {
        json kj;
        kj["frame"] = key.frame;
        kj["camera_mode"] = string(magic_enum::enum_name(key.cameraMode));
        kj["position"] = Utils::Vec3_ToJson(key.position);
        kj["rotation"] = Utils::Quat_ToJson(key.rotation);
        kj["fov_y"] = key.fovY;
        kj["ease_type"] = string(magic_enum::enum_name(key.easeType));

        // Target / LookAt 전용
        kj["target_tag"] = key.targetTag;
        kj["distance"] = key.distance;
        kj["target_offset"] = Utils::Vec3_ToJson(key.targetOffset);
        kj["pitch"] = key.pitch;
        kj["yaw"] = key.yaw;

        trackJson["keys"].push_back(kj);
    }

    root["track"] = trackJson;

    return root;
}

bool CameraTrack_Serializer::From_Json(const json& root, FCameraSequenceAsset& outAsset)
{
    outAsset = {};
    outAsset.name = root.value("name", "Untitled");

    if (!root.contains("track"))
        return false;

    const auto& tj = root["track"];

    outAsset.track.trackName = tj.value("track_name", "Camera");
    outAsset.track.fps = max(1, tj.value("fps", 30));
    outAsset.track.totalFrame = max(1, tj.value("total_frames", 300));

    if (tj.contains("keys") && tj["keys"].is_array())
    {
        for (const auto& kj : tj["keys"])
        {
            FCameraKey key;
            key.frame = kj.value("frame", 0);

            auto modeOpt = magic_enum::enum_cast<ECineCameraMode>(
                kj.value("camera_mode", "Free"));

            key.cameraMode = modeOpt.value_or(ECineCameraMode::Free);
            key.position = Utils::Vec3_FromJson(kj.value("position", json::array()));
            key.rotation = Utils::Quat_FromJson(kj.value("rotation", json::array()));
            key.fovY = kj.value("fov_y", XM_PIDIV4);

            auto easeOpt = magic_enum::enum_cast<ECameraEaseType>(
                kj.value("ease_type", "Linear"));

            key.easeType = easeOpt.value_or(ECameraEaseType::Linear);
            key.targetTag = kj.value("target_tag", "");
            key.distance = kj.value("distance", 10.f);
            key.targetOffset = Utils::Vec3_FromJson(kj.value("target_offset", json::array()),
                Vec3(0.f, 2.f, 0.f));

            key.pitch = kj.value("pitch", 0.f);
            key.yaw = kj.value("yaw", 0.f);
            outAsset.track.keys.push_back(key);
        }
    }
    return true;
}

bool CameraTrack_Serializer::Save_ToFile(const wstring& filePath, const FCameraSequenceAsset& asset)
{
    fs::create_directories(fs::path(filePath).parent_path());

    ofstream file(filePath);
    if (!file.is_open())
        return false;

    file << To_Json(asset).dump(4);

    return true;
}

bool CameraTrack_Serializer::Load_FromFile(const wstring& filePath, FCameraSequenceAsset& outAsset)
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

fs::path CameraTrack_Serializer::Get_BaseFolderPath()
{
    return fs::path("../../Client/Bin/Resources/Data/json/Cinematics");
}
