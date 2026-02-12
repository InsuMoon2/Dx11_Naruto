#include "pch.h"
#include "Behavior.h"
#include <fstream>
#include "Blackboard.h"
#include "BTComposite.h"
#include "BTNode.h"
#include "BTTask_Wait.h"

//REGISTER_COMPONENT_FACTORY(Behavior, Protocol::COMPONENT_TYPE_AI)

Behavior::Behavior(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

Behavior::Behavior(const Behavior& rhs)
    : Component(rhs)
    , _rootNode(rhs._rootNode)
    , _blackboard(rhs._blackboard)
{

}

Behavior::~Behavior()
{
}

HRESULT Behavior::Initialize_Prototype()
{
    Component::Initialize_Prototype();

    return S_OK;
}

HRESULT Behavior::Initialize(void* arg)
{
    _blackboard = Blackboard::Create();


    return S_OK;
}

void Behavior::Update(float timeDelta)
{
    if (!_rootNode)
        return;

    EBTNodeResult result = _rootNode->Update(timeDelta);

    if (result != EBTNodeResult::InProgress)
    {
        _rootNode->Initialize(); // 다음 프레임에 처음부터 다시 실행
    }
}

void Behavior::Set_RootNode(Shared<BTNode> rootNode)
{
    _rootNode = rootNode;

    if (_rootNode && _blackboard)
    {
        _rootNode->Set_Blackboard(_blackboard);
    }
}

void Behavior::Set_Blackboard(Shared<Blackboard> blackboard)
{
    _blackboard = blackboard;

    if (_rootNode)
    {
        _rootNode->Set_Blackboard(_blackboard);
    }
}

HRESULT Behavior::Load_FromJson(const wstring& filePath)
{
    ifstream file(filePath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to Open BT JSON");
        return E_FAIL;
    }

    json root;
    file >> root;

    // 노드 생성 및 ID 매핑
    map<int, Shared<BTNode>> nodeMap;
    int rootId = root["root_node_id"].get<int>();

    if (root.contains("nodes"))
    {
        for (const auto& nodeJson : root["nodes"])
        {
            int id = nodeJson["id"].get<int>();
            string type = nodeJson["type"].get<string>();

            // 타입에 맞는 노드 생성
            Shared<BTNode> newNode = Create_Node(type);
            if (!newNode) continue;

            // TODO : 파라미터 (Wait 시간 등) 로드 <- 아직 안됐음 내일할거
            // if (nodeJson.contains("parameters")) { ... }

            nodeMap[id] = newNode;
        }
    }

    // 부모 자식 연결. (Links 정보 이용)

    map<int, int> pinToNodeMap; // PinID -> NodeID로 매핑

    for (const auto& nodeJson : root["nodes"])
    {
        int nodeId = nodeJson["id"].get<int>();

        // Input Pin
        if (nodeJson.contains("input_pin_id"))
            pinToNodeMap[nodeJson["input_pin_id"].get<int>()] = nodeId;

        // Output Pins
        if (nodeJson.contains("output_pin_ids"))
        {
            for (const auto& pinId : nodeJson["output_pin_ids"])
                pinToNodeMap[pinId.get<int>()] = nodeId;
        }
    }

    // 링크 순회
    if (root.contains("links"))
    {
        for (const auto& linkJson : root["links"])
        {
            int startPin = linkJson["start"].get<int>(); // 부모의 Output Pin
            int endPin = linkJson["end"].get<int>();   // 자식의 Input Pin

            int parentNodeId = pinToNodeMap[startPin];
            int childNodeId = pinToNodeMap[endPin];

            Shared<BTNode> parent = nodeMap[parentNodeId];
            Shared<BTNode> child = nodeMap[childNodeId];

            if (parent && child)
            {
                // 부모가 Composite라면 자식 추가
                auto composite = dynamic_pointer_cast<BTComposite>(parent);
                if (composite)
                {
                    composite->Add_Child(child);
                }
            }
        }

    }

    // Root Node 설정
    if (nodeMap.contains(rootId))
    {
        Set_RootNode(nodeMap[rootId]);
    }

    // Blackboard 로드
    if (root.contains("blackboard") && _blackboard)
    {
        _blackboard->Deserialize_FromJson(root["blackboard"]);
    }

    return S_OK;
}

Shared<BTNode> Behavior::Create_Node(const string& typeName)
{
    // Composite
    if (typeName == "Sequence") return make_shared<BTSequence>();
    if (typeName == "Selector") return make_shared<BTSelector>();

    // Task
    if (typeName == "Task_Wait") return make_shared<BTTask_Wait>(1.0f);

    // 근데.. 클라에서 만들 테스크들은 어떻게할지?..
    // Behavior Component를 상속받아서 클라이언트 Behavior를 만들어서 사용하는게 좋을거같은데

    return nullptr;
}

Shared<Behavior> Behavior::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Behavior>(device, context);

    instance->Initialize_Prototype();

    return instance;
}

Shared<Component> Behavior::Clone(void* arg)
{
    // 기본 복사 생성자 호출
    auto instance = make_shared<Behavior>(*this);

    if (_rootNode)
    {
        // 트리를 통째로 복제하여 교체
        instance->Set_RootNode(_rootNode->Clone());
    }

    // 새로운 블랙보드 생성 후 세팅
    instance->_blackboard = Blackboard::Create();

    if (instance->_rootNode)
    {
        instance->_rootNode->Set_Blackboard(instance->_blackboard);
    }

    instance->Initialize(arg);

    return instance;
}
