#pragma once

#include "Base.h"
#include "WaveTrigger.h"

NS_BEGIN(Editor)

class WaveTrigger_Inspector final
{
public:
    explicit WaveTrigger_Inspector() = default;
    ~WaveTrigger_Inspector() = default;

public:
    bool Draw_Inspector(const Shared<WaveTrigger>& waveTrigger);

private:
    vector<string> Collect_AvailablePrefabNames() const;
    bool Draw_SpawnEntryPrefabSelector(WaveTrigger::FWaveSpawnEntry& entry, const vector<string>& prefabNames);
    bool Draw_SpawnEntryObjectType(WaveTrigger::FWaveSpawnEntry& entry);
    bool Draw_SpawnEntryTransform(WaveTrigger::FWaveSpawnEntry& entry);

private:
    char _prefabSearchBuf[256] = {};
};

NS_END
