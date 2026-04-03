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

    // 렌더 색 복구를 위해 한번 끄기
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

            if (src->Get_Owner() == dst->Get_Owner())
                continue;

            const auto srcCh = src->Get_Channel();
            const auto dstCh = dst->Get_Channel();

            if (!Can_Collide(srcCh, src->Get_CollisionMask(), dstCh, dst->Get_CollisionMask()))
            {
                continue;
            }

            // 충돌 검사 및 블로킹 세팅
            bool isIntersecting = false;

            Vec3 normal = Vec3::Zero; // 밀어낼 방향 src-> dst 기준
            float depth = 0.f;        // 얼마나 겹쳤는지

            if (Is_Blocking(src->Get_Channel(), dst->Get_Channel()))
            {
                isIntersecting = src->Intersect_WithDepth(dst, normal, depth);

                if (isIntersecting && depth > 0.0001)
                {
                    if (src->Get_Channel() == Collision_Channel::Player_Body)
                    {
                        if (auto ownerTransform = src->Get_Owner()->Get_Transform())
                            ownerTransform->Add_WorldOffset(-normal * depth);
                    }
                    else if (dst->Get_Channel() == Collision_Channel::Player_Body)
                    {
                        if (auto ownerTransform = dst->Get_Owner()->Get_Transform())
                            ownerTransform->Add_WorldOffset(normal * depth);
                    }
                }
                else
                {
                    // 일반적인 충돌 판정
                    isIntersecting = src->Intersect(dst);
                }
            }
            else
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
                    src->Get_Owner()->OnBeginOverlap(src, dst);
                    dst->Get_Owner()->OnBeginOverlap(dst, src);
                    src->Add_Overlap(dst);
                    dst->Add_Overlap(src);
                }
                else
                {
                    src->Get_Owner()->OnStayOverlap(src, dst);
                    dst->Get_Owner()->OnStayOverlap(dst, src);
                }
            }
            else
            {
                if (wasOverlapping)
                {
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
    {
        _isDebug = !_isDebug;
    }
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
