#pragma once

#include "Base.h"
#include "FMOD/fmod.h"

NS_BEGIN(Engine)

class ENGINE_DLL Sound_Manager : public Base
{
public:
    struct FFadeChannelInfo
    {
        FMOD_CHANNEL*  channel = nullptr;
        float          currentVolume = 0.f;
        float          fadeSpeed = 0.f;
        ESoundChannel  channelType = ESoundChannel::Effect;
    };

public:
    explicit Sound_Manager();
    virtual ~Sound_Manager() = default;

public:
    HRESULT Initialize(const wstring& soundRootPath, int32 maxChannels = 128);
    void    Update(float timeDelta);

public:
    // 1회 재생
    bool    Play_Once(const wstring& soundFile, ESoundChannel channel, float volume = 1.f);

    // 1회 재생 + 피치 조절
    bool    Play_Once_Pitched(const wstring& soundFile, ESoundChannel channel, float volume, float pitch);

    // BGM 반복 재생
    bool    Play_BGM(const wstring& soundFile,
        float volume = 1.f,
        bool stopPrevBGM = true,
        float fadeOutDuration = 0.f);

    // 일반 채널 반복 재생
    bool    Play_Loop(const wstring& soundFile,
        ESoundChannel channel,
        float volume = 1.f,
        bool stopPrevChannel = false,
        float fadeOutDuration = 0.f);

    void    Set_ChannelVolume(ESoundChannel channel, float volume);
    float   Get_ChannelVolume(ESoundChannel channel) const;

    void    Stop_Channel(ESoundChannel channel, float fadeOutDuration = 0.f);
    void    Stop_Sound(const wstring& soundFile);
    void    Stop_All(float fadeOutDuration = 0.f);

    bool    Has_Sound(const wstring& soundFile) const;

private:
    HRESULT Create_ChannelGroups();
    void    Load_SoundFiles();

    wstring Normalize_SoundKey(const wstring& soundFile) const;
    bool    Validate_Channel(ESoundChannel channel) const;

    FMOD_SOUND* Find_Sound(const wstring& soundFile) const;

    void    Register_ActiveChannel(ESoundChannel channel, FMOD_CHANNEL* fmodChannel);
    void    Cleanup_StoppedChannels();
    void    Cleanup_StoppedBGMChannels();

    void    Add_FadeChannel(FMOD_CHANNEL* fmodChannel, ESoundChannel channelType, float fadeOutDuration);
    void    Update_FadeChannels(float timeDelta);

    void    Release_AllSounds();
    void    Release_ChannelGroups();
    void    Shutdown_FMOD();

private:
    FMOD_SYSTEM* _system = nullptr;

    array<FMOD_CHANNELGROUP*, ETOI(ESoundChannel::END)> _channelGroups = {};
    array<float, ETOI(ESoundChannel::END)>              _channelVolumes = {};

    // 키는 Resources/Sounds 기준 상대 경로를 소문자로 정규화해서 저장
    umap<wstring, FMOD_SOUND*> _sounds;

    // 채널별 활성 재생 목록
    array<vector<FMOD_CHANNEL*>, ETOI(ESoundChannel::END)> _activeChannels = {};

    // BGM은 별도 추적해서 페이드아웃/교체 관리
    vector<FMOD_CHANNEL*> _bgmChannels;

    list<FFadeChannelInfo> _fadeChannels;

    wstring _soundRootPath;
    int32   _maxChannels = 128;

public:
    static Unique<Sound_Manager> Create(const wstring& soundRootPath, int32 maxChannels = 128);
    void Free() override;

};

NS_END
