#include "pch.h"
#include "Collision_Manager.h"
#include "Collision_Define.h"
#include "Collider.h"
#include "GameObject.h"
#include "Input_Manager.h"

HRESULT Collision_Manager::Initialize()
{
    _colliders.clear();

    return S_OK;
}

void Collision_Manager::Update()
{
    size_t colCount = _colliders.size();

    // 디버그 색상 리셋
    for (size_t i = 0; i < colCount; ++i)
    {
        auto collider = _colliders[i].lock();
        if (collider)
            collider->Set_IsColl(false);
    }

    for (size_t i = 0; i < colCount; i++)
    {
        auto src = _colliders[i].lock();
        if (!src || !src->Get_IsActive()) continue;

        for (size_t j = i + 1; j < colCount; ++j)
        {
            auto dst = _colliders[j].lock();
            if (!dst || !dst->Get_IsActive()) continue;

            // 같은 오브젝트 소속이면 스킵
            if (src->Get_Owner() == dst->Get_Owner())
                continue;

            ECollisionResponse response = Calculate_ResponseResult(
                src->Get_Channel(), src->Get_OverlapMask(), src->Get_BlockMask(),
                dst->Get_Channel(), dst->Get_OverlapMask(), dst->Get_BlockMask());

            // Ignore면 무시
            if (response == ECollisionResponse::Ignore)
                continue;

            bool isIntersecting = false;

            if (response == ECollisionResponse::Block)
            {
                Vec3 normal = Vec3::Zero;
                float depth = 0.f;
                isIntersecting = src->Intersect_WithDepth(dst, normal, depth);

                if (isIntersecting && depth > 0.0001f)
                {
                    auto srcOwner = src->Get_Owner();
                    auto dstOwner = dst->Get_Owner();

                    float srcRatio = 0.5f;
                    float dstRatio = 0.5f;

                    // Enviroment 채널이면 고정으로 간주
                    bool srcStatic = (src->Get_Channel() == Collision_Channel::Enviroment);
                    bool dstStatic = (dst->Get_Channel() == Collision_Channel::Enviroment);

                    bool srcIsPlayer  = (src->Get_Channel() == Collision_Channel::Player_Body);
                    bool dstIsPlayer  = (dst->Get_Channel() == Collision_Channel::Player_Body);
                    bool srcIsMonster = (src->Get_Channel() == Collision_Channel::Monster_Body);
                    bool dstIsMonster = (dst->Get_Channel() == Collision_Channel::Monster_Body);

                    // ── Player ↔ Monster: Soft Push (50:50, 감쇄) ──────────────────────
                    // 딱딱하게 튕기지 않고 서로 부드럽게 밀어낸다.
                    // depth에 감쇄 계수(0.4)를 곱해 급격한 위치 변화를 줄인다.
                    if ((srcIsPlayer && dstIsMonster) || (srcIsMonster && dstIsPlayer))
                    {
                        constexpr float SOFT_PUSH_DAMPING = 0.4f;
                        const float adjustedDepth = depth * SOFT_PUSH_DAMPING;

                        // 몬스터는 고정, 플레이어만 밀어냄 (몬스터 AI 경로가 흔들리지 않도록)
                        if (srcIsPlayer)
                        {
                            if (auto t = srcOwner->Get_Transform())
                                t->Add_WorldOffset(-normal * adjustedDepth);
                        }
                        else
                        {
                            if (auto t = dstOwner->Get_Transform())
                                t->Add_WorldOffset(normal * adjustedDepth);
                        }
                    }
                    // ── Enviroment vs 기타: 기존 로직 유지 ────────────────────────────
                    else if (srcStatic && !dstStatic)
                    {
                        srcRatio = 0.f;
                        dstRatio = 1.f;

                        if (auto t = dstOwner->Get_Transform())
                            t->Add_WorldOffset(normal * depth * dstRatio);
                    }
                    else if (!srcStatic && dstStatic)
                    {
                        srcRatio = 1.f;
                        dstRatio = 0.f;

                        if (auto t = srcOwner->Get_Transform())
                            t->Add_WorldOffset(-normal * depth * srcRatio);
                    }
                    else
                    {
                        // 기타 동적 vs 동적: 기존 50:50
                        if (auto t = srcOwner->Get_Transform())
                            t->Add_WorldOffset(-normal * depth * srcRatio);
                        if (auto t = dstOwner->Get_Transform())
                            t->Add_WorldOffset(normal * depth * dstRatio);
                    }
                }
            }
            else // Overlap
            {
                isIntersecting = src->Intersect(dst);
            }

            bool wasOverlapping = src->Is_Overlapping(dst);

            if (isIntersecting)
            {
                src->Set_IsColl(true);
                dst->Set_IsColl(true);

                if (!wasOverlapping)
                {
                    // Block / Overlap 구분하여 콜백 호출
                    if (response == ECollisionResponse::Block)
                    {
                        src->Get_Owner()->OnBlockBegin(src, dst);
                        dst->Get_Owner()->OnBlockBegin(dst, src);
                    }
                    else
                    {
                        src->Get_Owner()->OnBeginOverlap(src, dst);
                        dst->Get_Owner()->OnBeginOverlap(dst, src);
                    }

                    src->Add_Overlap(dst);
                    dst->Add_Overlap(src);
                }
                else
                {
                    // Stay
                    if (response == ECollisionResponse::Block)
                    {
                        src->Get_Owner()->OnBlockStay(src, dst);
                        dst->Get_Owner()->OnBlockStay(dst, src);
                    }
                    else
                    {
                        src->Get_Owner()->OnStayOverlap(src, dst);
                        dst->Get_Owner()->OnStayOverlap(dst, src);
                    }
                }
            }
            else
            {
                if (wasOverlapping)
                {
                    // End
                    src->Get_Owner()->OnEndOverlap(src, dst);
                    dst->Get_Owner()->OnEndOverlap(dst, src);

                    src->Remove_Overlap(dst);
                    dst->Remove_Overlap(src);
                }
            }
        }
    }

#ifdef _DEBUG
    if (INPUT->KeyDown(KEY_TYPE::F1))
        _isDebug = !_isDebug;
#endif
}

#ifdef _DEBUG
    void Collision_Manager::Render_Debug()
    {
        if (!_isDebug)
            return;

        for (const auto& colliderWeak : _colliders)
        {
            auto collider = colliderWeak.lock();

            if (collider)
            {
                collider->Render_Debug();
            }
        }
    }
#endif

    void Collision_Manager::Add_Collider(Shared<Collider> collider)
    {
        if (collider == nullptr)
            return;

        _colliders.push_back(collider);
    }

    void Collision_Manager::Clear_Colliders()
    {
        _colliders.clear();
    }

    Unique<Collision_Manager> Collision_Manager::Create()
    {
        auto collMgr = make_unique<Collision_Manager>();
        collMgr->Initialize();

        return collMgr;
    }

    void Collision_Manager::Free()
    {
        Base::Free();

        _colliders.clear();
    }
