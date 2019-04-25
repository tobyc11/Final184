#pragma once
#include "Camera.h"
#include "Light.h"
#include "Primitive.h"
#include <BoundingBox.h>
#include <Matrix3x4.h>
#include <Vector3.h>
#include <memory>
#include <string>

namespace Foreground
{

class CScene;

enum class ETransformSpace
{
    Local,
    Parent,
    World
};

class CSceneNode
{
public:
    CSceneNode(CScene* scene, CSceneNode* parent);

    void SetName(const std::string& name);
    std::string GetName() const;
    CSceneNode* CreateChildNode();
    void RemoveChildNode(CSceneNode* node);

    void SetPosition(const tc::Vector3& position);
    void SetRotation(const tc::Quaternion& rotation);
    void SetDirection(const tc::Vector3& direction);
    void SetScale(float scale);
    void SetScale(const tc::Vector3& scale);
    void SetTransform(const tc::Vector3& position, const tc::Quaternion& rotation);
    void SetTransform(const tc::Vector3& position, const tc::Quaternion& rotation, float scale);
    void SetTransform(const tc::Vector3& position, const tc::Quaternion& rotation,
                      const tc::Vector3& scale);
    void SetTransform(const tc::Matrix3x4& transform);

    void SetWorldPosition(const tc::Vector3& position);
    void SetWorldRotation(const tc::Quaternion& rotation);
    void SetWorldDirection(const tc::Vector3& direction);
    void SetWorldScale(float scale);
    void SetWorldScale(const tc::Vector3& scale);
    void SetWorldTransform(const tc::Vector3& position, const tc::Quaternion& rotation);
    void SetWorldTransform(const tc::Vector3& position, const tc::Quaternion& rotation,
                           float scale);
    void SetWorldTransform(const tc::Vector3& position, const tc::Quaternion& rotation,
                           const tc::Vector3& scale);

    void Translate(const tc::Vector3& delta, ETransformSpace space);
    void Rotate(const tc::Quaternion& delta, ETransformSpace space);
    void RotateAround(const tc::Vector3& point, const tc::Quaternion& delta, ETransformSpace space);
    void Pitch(float angle, ETransformSpace space);
    void Yaw(float angle, ETransformSpace space);
    void Roll(float angle, ETransformSpace space);
    bool LookAt(const tc::Vector3& target, const tc::Vector3& up, ETransformSpace space);
    void Scale(float scale);
    void Scale(const tc::Vector3& scale);

    const tc::Vector3& GetPosition() const;
    const tc::Quaternion& GetRotation() const;
    const tc::Vector3& GetScale() const;
    const tc::Matrix3x4& GetTransform() const;

    tc::Vector3 GetWorldPosition() const;
    tc::Quaternion GetWorldRotation() const;
    tc::Vector3 GetWorldScale() const;
    const tc::Matrix3x4& GetWorldTransform() const;

    void AddPrimitive(std::shared_ptr<CPrimitive> primitive);
    void AddLight(std::shared_ptr<CLight> light);
    void SetCamera(std::shared_ptr<CCamera> camera);

    const tc::BoundingBox& GetBoundingBox() const;
    tc::BoundingBox GetWorldBoundingBox() const;

private:
    void MarkLocalToWorldDirty(bool recursive = true);

private:
    std::string Name;
    CScene* Scene = nullptr;
    CSceneNode* Parent = nullptr;
    std::vector<std::unique_ptr<CSceneNode>> Children;

    tc::Vector3 Translation;
    tc::Quaternion Rotation;
    tc::Vector3 ScaleFactor;

    mutable bool bLocalToWorldDirty = true;
    mutable tc::Matrix3x4 LocalToWorld;

    std::vector<std::shared_ptr<CPrimitive>> Primitives;
    std::vector<std::shared_ptr<CLight>> Lights;
    std::shared_ptr<CCamera> Camera;

    mutable bool bBoundingBoxDirty = true;
    mutable tc::BoundingBox BoundingBox;
};

} /* namespace Foreground */
