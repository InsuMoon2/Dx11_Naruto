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

            // 두 콜라이더의 채널이 서로의 마스크에 포함돼있는지 체크
            if (!Can_Collide(src->Get_Channel(), src->Get_CollisionMask(),
                             dst->Get_Channel(), dst->Get_CollisionMask()))
            {
                continue;
            }

            // 충돌 검사
            bool isIntersecting = src->Intersect(dst);
            // 이전 프레임에서 src, dst가 서로 충돌했는지?
            bool wasOverlapping = src->Is_Overlapping(dst);

            if (isIntersecting)
            {
                src->Set_IsColl(true);
                dst->Set_IsColl(true);

                // 이전 프레임에서 충돌 안됐었다면,
                if (!wasOverlapping)
                {
                    // 최초 충돌
                    src->Get_Owner()->OnBeginOverlap(dst);
                    dst->Get_Owner()->OnBeginOverlap(src);

                    src->Add_Overlap(dst);
                    dst->Add_Overlap(src);
                }
                // 충돌 중이면
                else
                {
                    src->Get_Owner()->OnStayOverlap(dst);
                    dst->Get_Owner()->OnStayOverlap(src);
                }
            }
            else
            {
                if (wasOverlapping)
                {
                    src->Get_Owner()->OnEndOverlap(dst);
                    dst->Get_Owner()->OnEndOverlap(src);

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
