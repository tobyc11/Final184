#pragma once
#include "SceneNode.h"
#include "Octree.h"
#include "Scene.h"

namespace Foreground
{

static uint32_t NameCounter;

CSceneNode::CSceneNode(CScene* scene, CSceneNode* parent)
    : Name("unnamed" + std::to_string(NameCounter++))
    , Scene(scene)
    , Parent(parent)
    , ScaleFactor(tc::Vector3::ONE)
{
    // Whenever the scene node is inserted into a scene, put it into the accel structure
    if (Scene)
        Scene->GetAccelStructure()->InsertObject(this);
}

void CSceneNode::SetName(const std::string& name) { Name = name; }

std::string CSceneNode::GetName() const { return Name; }

CSceneNode* CSceneNode::CreateChildNode()
{
    auto child = std::make_unique<CSceneNode>(Scene, this);
    CSceneNode* weakPtr = child.get();
    Children.emplace_back(std::move(child));
    return weakPtr;
}

void CSceneNode::RemoveChildNode(CSceneNode* node)
{
    for (auto iter = Children.begin(); iter != Children.end(); ++iter)
    {
        if (iter->get() == node)
        {
            // Erase it from accel structure before deletion
            Scene->GetAccelStructure()->EraseObject(iter->get());
            Children.erase(iter);
            return;
        }
    }
    // NO such child
}

void CSceneNode::SetPosition(const tc::Vector3& position)
{
    Translation = position;
    MarkLocalToWorldDirty(true);
}

void CSceneNode::SetRotation(const tc::Quaternion& rotation)
{
    Rotation = rotation;
    MarkLocalToWorldDirty(true);
}

void CSceneNode::SetDirection(const tc::Vector3& direction)
{
    SetRotation(tc::Quaternion(tc::Vector3::FORWARD, direction));
}

void CSceneNode::SetScale(float scale) { SetScale(tc::Vector3(scale, scale, scale)); }

void CSceneNode::SetScale(const tc::Vector3& scale)
{
    ScaleFactor = scale;
    MarkLocalToWorldDirty(true);
}

void CSceneNode::SetTransform(const tc::Vector3& position, const tc::Quaternion& rotation)
{
    SetPosition(position);
    SetRotation(rotation);
}

void CSceneNode::SetTransform(const tc::Vector3& position, const tc::Quaternion& rotation,
                              float scale)
{
    SetPosition(position);
    SetRotation(rotation);
    SetScale(scale);
}

void CSceneNode::SetTransform(const tc::Vector3& position, const tc::Quaternion& rotation,
                              const tc::Vector3& scale)
{
    SetPosition(position);
    SetRotation(rotation);
    SetScale(scale);
}

void CSceneNode::SetTransform(const tc::Matrix3x4& transform)
{
    SetTransform(transform.Translation(), transform.Rotation(), transform.Scale());
}

void CSceneNode::SetWorldPosition(const tc::Vector3& position)
{
    if (!Parent)
        SetPosition(position);
    else
        SetPosition(position - Parent->GetWorldPosition());
}

void CSceneNode::SetWorldRotation(const tc::Quaternion& rotation)
{
    if (!Parent)
        SetRotation(rotation);
    else
        SetRotation(Parent->GetWorldRotation().Inverse() * rotation);
}

void CSceneNode::SetWorldDirection(const tc::Vector3& direction)
{
    tc::Vector3 localDirection = direction;
    if (Parent)
        localDirection = Parent->GetWorldRotation().Inverse() * direction;
    SetDirection(localDirection);
}

void CSceneNode::SetWorldScale(float scale) { SetWorldScale(tc::Vector3(scale, scale, scale)); }

void CSceneNode::SetWorldScale(const tc::Vector3& scale)
{
    if (!Parent)
        SetScale(scale);
    else
        SetScale(scale / Parent->GetWorldScale());
}

void CSceneNode::SetWorldTransform(const tc::Vector3& position, const tc::Quaternion& rotation)
{
    if (!Parent)
        SetTransform(position, rotation);
    else
        SetTransform(Parent->GetWorldTransform().Inverse()
                     * tc::Matrix3x4(position, rotation, 1.0f));
}

void CSceneNode::SetWorldTransform(const tc::Vector3& position, const tc::Quaternion& rotation,
                                   float scale)
{
    if (!Parent)
        SetTransform(position, rotation, scale);
    else
        SetTransform(Parent->GetWorldTransform().Inverse()
                     * tc::Matrix3x4(position, rotation, scale));
}

void CSceneNode::SetWorldTransform(const tc::Vector3& position, const tc::Quaternion& rotation,
                                   const tc::Vector3& scale)
{
    if (!Parent)
        SetTransform(position, rotation, scale);
    else
        SetTransform(Parent->GetWorldTransform().Inverse()
                     * tc::Matrix3x4(position, rotation, scale));
}

void CSceneNode::Translate(const tc::Vector3& delta, ETransformSpace space)
{
    throw "unimplemented";
}

void CSceneNode::Rotate(const tc::Quaternion& delta, ETransformSpace space)
{
    throw "unimplemented";
}

void CSceneNode::RotateAround(const tc::Vector3& point, const tc::Quaternion& delta,
                              ETransformSpace space)
{
    throw "unimplemented";
}

void CSceneNode::Pitch(float angle, ETransformSpace space) { throw "unimplemented"; }

void CSceneNode::Yaw(float angle, ETransformSpace space) { throw "unimplemented"; }

void CSceneNode::Roll(float angle, ETransformSpace space) { throw "unimplemented"; }

bool CSceneNode::LookAt(const tc::Vector3& target, const tc::Vector3& up, ETransformSpace space)
{
    throw "unimplemented";
}

void CSceneNode::Scale(float scale) { SetScale(GetScale() * scale); }

void CSceneNode::Scale(const tc::Vector3& scale) { SetScale(GetScale() * scale); }

const tc::Vector3& CSceneNode::GetPosition() const { return Translation; }

const tc::Quaternion& CSceneNode::GetRotation() const { return Rotation; }

const tc::Vector3& CSceneNode::GetScale() const { return ScaleFactor; }

const tc::Matrix3x4& CSceneNode::GetTransform() const
{
    return tc::Matrix3x4(Translation, Rotation, ScaleFactor);
}

tc::Vector3 CSceneNode::GetWorldPosition() const { return GetWorldTransform().Translation(); }

tc::Quaternion CSceneNode::GetWorldRotation() const { return GetWorldTransform().Rotation(); }

tc::Vector3 CSceneNode::GetWorldScale() const { return GetWorldTransform().Scale(); }

const tc::Matrix3x4& CSceneNode::GetWorldTransform() const
{
    if (bLocalToWorldDirty)
    {
        if (!Parent)
            LocalToWorld = GetTransform();
        else
            LocalToWorld = Parent->GetWorldTransform() * GetTransform();

        if (Scene)
            Scene->GetAccelStructure()->UpdateObject(const_cast<CSceneNode*>(this));
        bLocalToWorldDirty = false;
    }
    return LocalToWorld;
}

void CSceneNode::AddPrimitive(std::shared_ptr<CPrimitive> primitive) { bBoundingBoxDirty = true; }

void CSceneNode::AddLight(std::shared_ptr<CLight> light) { Lights.push_back(light); }

void CSceneNode::SetCamera(std::shared_ptr<CCamera> camera) { Camera = camera; }

const tc::BoundingBox& CSceneNode::GetBoundingBox() const
{
    if (bBoundingBoxDirty)
    {
        tc::BoundingBox bb;
        for (const auto& primitive : Primitives)
        {
            auto primBb = primitive->GetBoundingBox();
            bb.Merge(primBb);
        }
        BoundingBox = bb;
        if (Scene)
            Scene->GetAccelStructure()->UpdateObject(const_cast<CSceneNode*>(this));
        bBoundingBoxDirty = false;
    }
    return BoundingBox;
}

tc::BoundingBox CSceneNode::GetWorldBoundingBox() const
{
    return GetBoundingBox().Transformed(GetWorldTransform());
}

void CSceneNode::MarkLocalToWorldDirty(bool recursive)
{
    bLocalToWorldDirty = true;
    if (recursive)
    {
        for (const auto& ptr : Children)
            ptr->MarkLocalToWorldDirty(recursive);
    }
}

} /* namespace Foreground */
