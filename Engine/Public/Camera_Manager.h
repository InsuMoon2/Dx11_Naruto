#pragma once

#include "Base.h"
#include "Delegate.h"

NS_BEGIN(Engine)
class Camera;
class CameraTrack_Player; 
class Camera_Cinematic;   
struct FCameraSequenceAsset;
class Transform;

class ENGINE_DLL Camera_Manager : public Base
{
public:
    explicit Camera_Manager() = default;
    virtual ~Camera_Manager() = default;

public:
    void            Update(float timeDelta);

    void            Set_ActiveCamera(Shared<Camera> camera);
    Shared<Camera>  Get_ActiveCamera() const { return _activeCamera.lock(); }

    bool            Is_ActiveCamera(const Shared<Camera> camera) const;

    void            Toggle_Camera();
    void            Register_Camera(Shared<Camera> camera);

    Shared<Camera>  Find_Camera(Protocol::OBJECT_TYPE type);

public:
    void            Clear_InvalidCameras();

public: /* Camera Shake */
    void Request_CameraShake(const FCameraShakeDesc& request);
    void Stop_CameraShake(const string& tag = "");
    void Clear_CameraShake();


private:
    Shared<Camera>  Find_NextValidCamera(const Shared<Camera>& current);

public: /* 시네마틱 */
    bool            Play_Cinematic(const wstring& sequenceName);
    bool            Play_Cinematic(const wstring& sequenceName,
                        Shared<Transform> anchorTransform,
                        bool blockGameInput);

    void            Stop_Cinematic();
    bool            Is_CinematicPlaying() const { return _isCinematicPlaying; }
    Shared<Camera_Cinematic> Get_CinematicCamera() { return _cineCamera; }


private:
    vector<Weak<Camera>>            _cameras;
    Weak<Camera>                    _activeCamera;

    Weak<Transform>                 _cineAnchorTransform;
private:
    Shared<CameraTrack_Player>      _cinePlayer;
    Shared<Camera_Cinematic>        _cineCamera;
    Weak<Camera>                    _originCamera;

    Unique<FCameraSequenceAsset>    _currentAsset;

    bool                            _isCinematicPlaying = false;
    bool                            _blockGameInputOnCinematic = false;


public:
    static Unique<Camera_Manager> Create();
    virtual void Free() override;

};

NS_END
