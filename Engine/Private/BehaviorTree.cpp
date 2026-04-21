#include "pch.h"
#include "BehaviorTree.h"
#include <fstream>
#include "Blackboard.h"
#include "BTComposite.h"
#include "BTDecorator.h"
#include "BTNode.h"
#include "BTTask_Wait.h"
#include "GameInstance.h"

BehaviorTree::BehaviorTree(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

BehaviorTree::BehaviorTree(const BehaviorTree& rhs)
    : Component(rhs)
    , _rootNode(rhs._rootNode)
    , _blackboard(rhs._blackboard)
{

}

BehaviorTree::~BehaviorTree()
{
}

HRESULT BehaviorTree::Initialize_Prototype()
{
    Component::Initialize_Prototype();

    return S_OK;
}

HRESULT BehaviorTree::Initialize(void* arg)
{
    if (!_blackboard)
        _blackboard = Blackboard::Create();

    if (_rootNode && _blackboard)
        _rootNode->Set_Blackboard(_blackboard);


    return S_OK;
}

void BehaviorTree::BeginPlay()
{
    Component::BeginPlay();

    auto owner = Get_Owner();
    if (_rootNode && owner)
        _rootNode->Set_Owner(owner);
}

void BehaviorTree::Update(float timeDelta)
{
    if (!_rootNode)
        return;

    if (_pendingInitialize)
    {
        _rootNode->Initialize();
        _pendingInitialize = false;
    }

    EBTNodeResult result = _rootNode->Update(timeDelta);

    if (result != EBTNodeResult::InProgress)
    {
        _pendingInitialize = true; // 다음 프레임에 리셋될 수 있게
    }

    //LOG_INFO();
}

json BehaviorTree::To_Json() const
{
    json j = Component::To_Json();

    j["bt_guid"] = _btGuid;

    return j;
}

void BehaviorTree::From_Json(const json& data)
{
    Component::From_Json(data);

    //_btFilePath = data.value("bt_filepath", string("(None)"));
    string guid = data.value("bt_guid", string(""));

    if (!guid.empty())
    {
        // GUID -> 절대경로 변환 후 로드
        wstring resolvedPath = GAME->Resolve_AssetPath(guid);
        if (!resolvedPath.empty())
        {
            _btGuid = guid;
            Load_FromJson(resolvedPath);
        }
        else
        {
            LOG_WARN("BT GUID could not be resolved: {}", guid);
        }
    }
    else
    {
        // 구버전 호환용. 추후 삭제예정
        string filePath = data.value("bt_filepath", string("(None)"));
        if (filePath != "(None)" && !filePath.empty())
        {
            Load_FromJson(Utils::ToWString(filePath));
        }
    }
}

void BehaviorTree::Set_RootNode(Shared<BTNode> rootNode)
{
    _rootNode = rootNode;

    if (_rootNode && _blackboard)
        _rootNode->Set_Blackboard(_blackboard);

    auto owner = Get_Owner();
    if (_rootNode && owner)
        _rootNode->Set_Owner(owner);
    
}

void BehaviorTree::Set_Blackboard(Shared<Blackboard> blackboard)
{
    _blackboard = blackboard;

    if (_rootNode)
    {
        _rootNode->Set_Blackboard(_blackboard);
    }
}

HRESULT BehaviorTree::Load_FromJson(const wstring& filePath)
{
    ifstream file(filePath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to Open BT JSON");
        return E_FAIL;
    }

    _btFilePath = Utils::ToString(filePath);

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

            Shared<BTNode> newNode = Create_Node(type);

            if (!newNode) continue;

            newNode->Set_DebugId(id);
            newNode->Set_Name(nodeJson.value("name", "Unknown"));
            newNode->Deserialize_FromJson(nodeJson);

            nodeMap[id] = newNode;
        }
    }

    // Pin -> NodeID 매핑
    map<int, int> pinToNodeMap;

    for (const auto& nodeJson : root["nodes"])
    {
        int nodeId = nodeJson["id"].get<int>();

        if (nodeJson.contains("input_pin_id"))
            pinToNodeMap[nodeJson["input_pin_id"].get<int>()] = nodeId;

        if (nodeJson.contains("output_pin_ids"))
        {
            for (const auto& pinId : nodeJson["output_pin_ids"])
            {
                pinToNodeMap[pinId.get<int>()] = nodeId;
            }
        }
    }

    // Output Pin
    map<int, int> outputPinOrder;
    for (const auto& nodeJson : root["nodes"])
    {
        if (nodeJson.contains("output_pin_ids"))
        {
            int order = 0;

            for (const auto& pinId : nodeJson["output_pin_ids"])
            {
                outputPinOrder[pinId.get<int>()] = order++;
            }
        }
    }
    
    map<int, vector<pair<int, Shared<BTNode>>>> compositeChildren;

    if (root.contains("links"))
    {
        for (const auto& linkJson : root["links"])
        {
            int startPin = linkJson["start"].get<int>();
            int endPin = linkJson["end"].get<int>();

            int parentNodeId = pinToNodeMap[startPin];
            int childNodeId = pinToNodeMap[endPin];

            Shared<BTNode> parent = nodeMap[parentNodeId];
            Shared<BTNode> child = nodeMap[childNodeId];

            if (parent && child)
            {
                auto composite = dynamic_pointer_cast<BTComposite>(parent);
                auto decorator = dynamic_pointer_cast<BTDecorator>(parent);

                if (composite)
                {
                    int order = outputPinOrder.count(startPin) ? outputPinOrder[startPin] : 0;
                    compositeChildren[parentNodeId].push_back({ order, child });
                }
                else if (decorator)
                {
                    decorator->Set_Child(child);
                }
            }
        }
    }

    // 정렬 후 Add
    for (auto& [parentId, children] : compositeChildren)
    {
        sort(children.begin(), children.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

        auto composite = dynamic_pointer_cast<BTComposite>(nodeMap[parentId]);

        for (auto& [order, child] : children)
        {
            LOG_INFO("Add_Child order={}, debugId={}", order, child->Get_DebugId());

            composite->Add_Child(child);
        }
    }

    // Root Node 설정
    if (nodeMap.contains(rootId))
        Set_RootNode(nodeMap[rootId]);

    // Blackboard 로드
    if (root.contains("blackboard") && _blackboard)
        _blackboard->Deserialize_FromJson(root["blackboard"]);

    return S_OK;
}

HRESULT BehaviorTree::Reload_FromBoundAsset()
{
    if (!_btGuid.empty())
    {
        const wstring resolvedPath = GAME->Resolve_AssetPath(_btGuid);
        if (resolvedPath.empty())
        {
            LOG_WARN("BehaviorTree reload skipped because the bound guid could not be resolved: {}", _btGuid);
            return E_FAIL;
        }

        CHECK_FAILED(Load_FromJson(resolvedPath), E_FAIL);
        _pendingInitialize = true;
        _cachedNodeResults.clear();
        return S_OK;
    }

    if (_btFilePath != "(None)" && !_btFilePath.empty())
    {
        CHECK_FAILED(Load_FromJson(Utils::ToWString(_btFilePath)), E_FAIL);
        _pendingInitialize = true;
        _cachedNodeResults.clear();
        return S_OK;
    }

    LOG_WARN("BehaviorTree reload skipped because no bound asset path exists.");
    return E_FAIL;
}

map<int, EBTNodeResult> BehaviorTree::Get_AllNodeResults() const
{
    map<int, EBTNodeResult> result;

    if (_rootNode)
    {
        _rootNode->Gather_NodeResults(result);
    }

    return result;
}

Shared<BTNode> BehaviorTree::Create_Node(const string& typeName)
{
    return GAME->Instantiate_BTNode(typeName);
}

Shared<BehaviorTree> BehaviorTree::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<BehaviorTree>(device, context);

    instance->Initialize_Prototype();

    return instance;
}

Shared<Component> BehaviorTree::Clone(void* arg)
{
    auto instance = make_shared<BehaviorTree>(*this);

    if (_rootNode)
    {
        instance->Set_RootNode(_rootNode->Clone());
    }

    instance->_blackboard = Blackboard::Create();

    if (instance->_rootNode)
    {
        instance->_rootNode->Set_Blackboard(instance->_blackboard);
    }

    instance->Initialize(arg);

    return instance;
}
