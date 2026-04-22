#include "pch.h"
#include "AIController.h"

#include "AnimationStateComponent.h"
#include "BehaviorTree.h"
#include "MovementComponent.h"
#include "GameObject.h"
#include "Blackboard.h"

static bool Is_PrefabPreviewPawn(const Shared<GameObject>& pawn)
{
    if (!pawn)
        return false;

    return pawn->Get_LevelIndex() == ETOI(ELevelType::Prefab);
}

AIController::AIController(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Controller(device, context)
{
}

AIController::AIController(const AIController& rhs)
    : Controller(rhs)
{
}

AIController::~AIController()
{
}

HRESULT AIController::Initialize_Prototype()
{
    return Controller::Initialize_Prototype();
}

HRESULT AIController::Initialize(void* arg)
{
    return Controller::Initialize(arg);
}

void AIController::BeginPlay()
{
    Controller::BeginPlay();

    auto pawn = Get_Pawn();
    if (!pawn)
        return;

    _movement = pawn->Get_Component<MovementComponent>();
    _behavior = pawn->Get_Component<BehaviorTree>();
    _animationState = pawn->Get_Component<AnimationStateComponent>();
 
    if (Is_PrefabPreviewPawn(pawn))
        return;

    if (_behavior && !_btFilePath.empty())
        _behavior->Load_FromJson(_btFilePath);

    Refresh_RuntimeBindings();

}

void AIController::Refresh_RuntimeBindings()
{
    auto pawn = Get_Pawn();
    if (!pawn)
    {
        _movement.reset();
        _behavior.reset();
        _animationState.reset();
        _blackboard.reset();
        _lastAnimState.clear();
        _lastAnimDirection = static_cast<int32>(EMoveInputDirection::Forward);
        _lastAnimReplaySerial = 0;

        return;
    }

    _movement = pawn->Get_Component<MovementComponent>();
    _behavior = pawn->Get_Component<BehaviorTree>();
    _animationState = pawn->Get_Component<AnimationStateComponent>();

    _blackboard.reset();
    if (_behavior)
        _blackboard = _behavior->Get_Blackboard();

    _lastAnimState.clear();
    _lastAnimDirection = static_cast<int32>(EMoveInputDirection::Forward);
    _lastAnimReplaySerial = 0;
}

void AIController::Update(float timeDelta)
{
    Controller::Update(timeDelta);
   
    if (Is_PrefabPreviewPawn(Get_Pawn()))
        return;

    if (_behavior)
        _behavior->Update(timeDelta);

    if (_movement && _blackboard)
    {
        MovementComponent::FMoveCommand command;

        if (_blackboard->HasKey("MoveAxisX"))
            command.moveAxis.x = _blackboard->Get_ValueAsFloat("MoveAxisX");

        if (_blackboard->HasKey("MoveAxisY"))
            command.moveAxis.y = _blackboard->Get_ValueAsFloat("MoveAxisY");

        if (_blackboard->HasKey("Sprint"))
            command.sprint = _blackboard->Get_ValueAsBool("Sprint");

        _movement->Apply_Command(command);
        _movement->Update(timeDelta);
    }

    if (_animationState && _blackboard)
    {
        string animState = _blackboard->HasKey("AnimState")
            ? _blackboard->Get_ValueAsString("AnimState")
            : "";

        EMoveInputDirection dir = EMoveInputDirection::Forward;
        if (_blackboard->HasKey("AnimDirection"))
        {
            dir = static_cast<EMoveInputDirection>(_blackboard->Get_ValueAsInt("AnimDirection"));
        }

        // 같은 AnimState여도, 다시 재생할 때 seiral 검색
        const int32 replaySerial = _blackboard->HasKey("AnimReplaySerial")
            ? _blackboard->Get_ValueAsInt("AnimReplaySerial") : 0;

        if (!animState.empty())
        {
            const auto* stateDesc = _animationState->Find_State(animState);
            if (stateDesc)
            {
                const int32 dirValue = static_cast<int32>(dir);

                const bool shouldReplay =
                    (animState != _lastAnimState) ||
                    (dirValue != _lastAnimDirection) ||
                    (replaySerial != _lastAnimReplaySerial);

                if (shouldReplay)
                {
                    if (stateDesc->mode == EStateAnimationMode::DirectionalSingle)
                        _animationState->Play_DirectionalState(animState, dir);
                    else
                        _animationState->Play_State(animState);

                    _lastAnimState = animState;
                    _lastAnimDirection = dirValue;
                    _lastAnimReplaySerial = replaySerial;
                }
            }

            if (_blackboard->HasKey("AnimRequestEnd") &&
                    _blackboard->Get_ValueAsBool("AnimRequestEnd"))
            {
                _animationState->Request_StateEnd();
                _blackboard->Set_ValueAsBool("AnimRequestEnd", false);
            }
        }

    }

}

json AIController::To_Json() const
{
    json j = Controller::To_Json();

    if (_btFilePath.length() > 0)
        j["bt_path"] = Utils::ToString(_btFilePath);

    return j;
}

void AIController::From_Json(const json& data)
{
    Controller::From_Json(data);

    if (data.contains("bt_path"))
    {
        wstring path = Utils::ToWString(data["bt_path"].get<string>());
        _btFilePath = path;

        if (_behavior)
            _behavior->Load_FromJson(path);
    }
}

Shared<AIController> AIController::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<AIController>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create: AIController");
        instance.reset();
    }

    return instance;
}

shared_ptr<Component> AIController::Clone(void* arg)
{
    auto clone = make_shared<AIController>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone: AIController");
        clone.reset();
    }

    return clone;
}

void AIController::Free()
{
    Controller::Free();
}
