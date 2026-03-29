# Anim Notify 다중 트랙 플랜

## 목표
- `Notify` 트랙 다중화
- `Notify State` 트랙 다중화
- 기존 `.animnotify.json` 로드 호환 유지
- 이번 단계는 리플렉션/단축키 제외

## 수정 파일
- `Engine/Public/AnimNotify_Types.h`
- `Engine/Private/AnimNotify_Serializer.cpp`
- `Editor/Public/Editor_Struct.h`
- `Editor/Public/Animation_View.h`
- `Editor/Private/Animation_View.cpp`
- `Editor/Public/AnimSequencerAdapter.h`
- `Editor/Private/AnimSequencerAdapter.cpp`

## 1. 데이터 구조
`trackIndex`와 트랙 메타를 추가한다.

```cpp
// Engine/Public/AnimNotify_Types.h
struct FAnimNotifyTrackDesc
{
    // 에디터에서 표시할 트랙 이름이다.
    string name = "";
};

struct FAnimNotifyEventEntry
{
    float timeSec = 0.f;
    // 이 Notify가 속한 트랙 인덱스다.
    int32 trackIndex = 0;
    Shared<AnimNotify> notify;
};

struct FAnimNotifyStateEntry
{
    float startSec = 0.f;
    float durationSec = 0.f;
    // 이 Notify State가 속한 트랙 인덱스다.
    int32 trackIndex = 0;
    Shared<AnimNotifyState> notifyState;
};

struct FAnimNotifyClipData
{
    string clipName;
    int32 displayFps = 30;

    // 이벤트형 Notify 트랙 목록이다.
    vector<FAnimNotifyTrackDesc> notifyTracks;
    // 구간형 Notify State 트랙 목록이다.
    vector<FAnimNotifyTrackDesc> notifyStateTracks;

    vector<FAnimNotifyEventEntry> notifies;
    vector<FAnimNotifyStateEntry> notifyStates;
};
```

## 2. Serializer
구버전에는 트랙 정보가 없으므로 로드시 기본 트랙을 만든다.

```cpp
// Engine/Private/AnimNotify_Serializer.cpp
static void Ensure_DefaultNotifyTracks(FAnimNotifyClipData& clip)
{
    if (clip.notifyTracks.empty())
    {
        FAnimNotifyTrackDesc trackDesc;
        trackDesc.name = "Notifies";
        clip.notifyTracks.push_back(trackDesc);
    }

    if (clip.notifyStateTracks.empty())
    {
        FAnimNotifyTrackDesc trackDesc;
        trackDesc.name = "Notify States";
        clip.notifyStateTracks.push_back(trackDesc);
    }
}

// 저장 예시
clipJson["notify_tracks"] = json::array();
clipJson["notify_state_tracks"] = json::array();

for (const auto& track : clip.notifyTracks)
    clipJson["notify_tracks"].push_back({ { "name", track.name } });

for (const auto& track : clip.notifyStateTracks)
    clipJson["notify_state_tracks"].push_back({ { "name", track.name } });

clipJson["notifies"].push_back({
    { "time_sec", entry.timeSec },
    { "track_index", entry.trackIndex },
    { "type", entry.notify->Get_TypeName() },
    { "payload", entry.notify->Serialize_Payload() }
});

// 로드 예시
entry.trackIndex = std::clamp(
    notifyJson.value("track_index", 0),
    0,
    static_cast<int32>(clip.notifyTracks.size()) - 1);
```

## 3. 에디터 컨텍스트
row가 Notify용인지 State용인지 알아야 한다.

```cpp
// Editor/Public/Editor_Struct.h
enum class EAnimNotifyTrackKind : uint8
{
    Notify,
    NotifyState
};

struct FAnimSequencerContext
{
    Animation_View* view = nullptr;
    FAnimNotifyClipData* clip = nullptr;

    int32* selectedNotifyIndex = nullptr;
    int32* selectedStateIndex = nullptr;

    // 현재 선택된 row의 종류다.
    EAnimNotifyTrackKind selectedTrackKind = EAnimNotifyTrackKind::Notify;
    // 현재 선택된 row의 실제 트랙 인덱스다.
    int32 selectedTrackIndex = 0;

    int32 pendingMarkStartFrame = -1;
    int32 pendingMarkEndFrame = -1;
    bool isMarkingState = false;
    bool clickedOnNotify = false;
};
```

## 4. Animation_View
선택된 트랙에 엔트리를 넣도록 바꾼다.

```cpp
// Editor/Public/Animation_View.h
class Animation_View : public EditorWindow
{
public:
    void Add_NotifyTrack(const string& trackName);
    void Add_NotifyStateTrack(const string& trackName);

private:
    // 현재 선택된 Notify 트랙 인덱스다.
    int32 _selectedNotifyTrackIndex = 0;
    // 현재 선택된 Notify State 트랙 인덱스다.
    int32 _selectedNotifyStateTrackIndex = 0;

    // 새 Notify 트랙 이름 입력 버퍼다.
    string _newNotifyTrackName = "Notify Track";
    // 새 Notify State 트랙 이름 입력 버퍼다.
    string _newNotifyStateTrackName = "Notify State Track";
};
```

```cpp
// Editor/Private/Animation_View.cpp
void Animation_View::Add_NotifyTrack(const string& trackName)
{
    auto* clip = Get_CurrentClip();
    if (!clip)
        return;

    FAnimNotifyTrackDesc trackDesc;
    trackDesc.name = trackName.empty() ? "Notify Track" : trackName;
    clip->notifyTracks.push_back(trackDesc);

    _selectedNotifyTrackIndex = static_cast<int32>(clip->notifyTracks.size()) - 1;
    _sequencerContext.selectedTrackKind = EAnimNotifyTrackKind::Notify;
    _sequencerContext.selectedTrackIndex = _selectedNotifyTrackIndex;
    MarkDirty();
}

void Animation_View::Add_Notify_AtFrame(int32 frame, const string& typeName)
{
    auto* clip = Get_CurrentClip();
    if (!clip)
        return;

    auto instance = AnimNotify_Factory::Create_Notify(typeName);
    if (!instance)
        return;

    FAnimNotifyEventEntry entry;
    entry.timeSec = static_cast<float>(frame) / static_cast<float>(Get_CurrentClipFps());
    entry.trackIndex = std::clamp(_selectedNotifyTrackIndex, 0, static_cast<int32>(clip->notifyTracks.size()) - 1);
    entry.notify = instance;
    clip->notifies.push_back(entry);
    MarkDirty();
}
```

생성 패널에는 아래 UI를 추가한다.

```cpp
void Animation_View::Draw_NotifyTrackSection()
{
    ImGui::Text("Notify Tracks");
    ImGui::Separator();

    char buffer[128] = {};
    strcpy_s(buffer, _newNotifyTrackName.c_str());
    if (ImGui::InputText("Track Name##NotifyTrack", buffer, static_cast<size_t>(std::size(buffer))))
        _newNotifyTrackName = buffer;

    if (ImGui::Button("Add Notify Track"))
        Add_NotifyTrack(_newNotifyTrackName);
}
```

## 5. AnimSequencerAdapter
현재 `2개 고정 row`를 동적 row로 바꾼다.

```cpp
// Editor/Public/AnimSequencerAdapter.h
struct FResolvedTrackRow
{
    // 현재 row의 트랙 종류다.
    EAnimNotifyTrackKind kind = EAnimNotifyTrackKind::Notify;
    // 현재 row의 실제 트랙 인덱스다.
    int32 trackIndex = 0;
};

bool Resolve_TrackRow(int32 rowIndex, FResolvedTrackRow& outRow) const;
void Draw_NotifyTrack(int32 trackIndex, ImDrawList* drawList, const ImRect& rc);
void Draw_StateTrack(int32 trackIndex, ImDrawList* drawList, const ImRect& rc);
```

```cpp
// Editor/Private/AnimSequencerAdapter.cpp
int AnimSequencerAdapter::GetItemCount() const
{
    auto* clip = Get_Clip();
    if (!clip)
        return 0;

    return static_cast<int>(clip->notifyTracks.size() + clip->notifyStateTracks.size());
}

bool AnimSequencerAdapter::Resolve_TrackRow(int32 rowIndex, FResolvedTrackRow& outRow) const
{
    auto* clip = Get_Clip();
    if (!clip || rowIndex < 0)
        return false;

    const int32 notifyTrackCount = static_cast<int32>(clip->notifyTracks.size());
    if (rowIndex < notifyTrackCount)
    {
        outRow.kind = EAnimNotifyTrackKind::Notify;
        outRow.trackIndex = rowIndex;
        return true;
    }

    const int32 stateRowIndex = rowIndex - notifyTrackCount;
    if (stateRowIndex < 0 || stateRowIndex >= static_cast<int32>(clip->notifyStateTracks.size()))
        return false;

    outRow.kind = EAnimNotifyTrackKind::NotifyState;
    outRow.trackIndex = stateRowIndex;
    return true;
}

void AnimSequencerAdapter::CustomDraw(int index, ImDrawList* drawList, const ImRect& rc, const ImRect&, const ImRect&, const ImRect&)
{
    FResolvedTrackRow row;
    if (!Resolve_TrackRow(index, row))
        return;

    if (row.kind == EAnimNotifyTrackKind::Notify)
        Draw_NotifyTrack(row.trackIndex, drawList, rc);
    else
        Draw_StateTrack(row.trackIndex, drawList, rc);
}
```

각 draw 함수에서는 `entry.trackIndex != trackIndex` 조건으로 해당 row에 속한 엔트리만 그린다.

## 구현 순서
1. `AnimNotify_Types.h` 구조 변경
2. serializer 저장/로드 호환 추가
3. `Animation_View` 트랙 생성/선택 상태 추가
4. `AnimSequencerAdapter` 동적 row 전환
5. 좌측 리스트를 트랙별 그룹으로 정리

## 주의사항
- 정렬은 `trackIndex -> timeSec`, `trackIndex -> startSec -> durationSec` 기준으로 맞춘다.
- 런타임 `Model.cpp`는 건드리지 않는 방향이 안전하다.
- 이번 단계는 트랙 추가/저장/로드/편집까지만 목표로 한다.
