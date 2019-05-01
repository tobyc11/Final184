#pragma once
#include <Frustum.h>
#include <Matrix4.h>

namespace Foreground
{

// The camera only defines the projection matrix, the view matrix is determined by the scene node in
// which this camera is located
class CCamera
{
public:
    CCamera(bool ortho = false);

    // The aspect ratio. Also the 2 members below are valid only for perspective
    void SetAspectRatio(float value);
    float GetAspectRatio() const;

    // The vertical field of view, the default is 59 degrees
    void SetFovY(float value);
    float GetFovY() const;

    void SetNearClip(float value);
    void SetFarClip(float value);
    float GetNearClip() const;
    float GetFarClip() const;

    // The magnifying constants are only used for orthographics projection
    void SetMagX(float value);
    void SetMagY(float value);
    float GetMagX() const;
    float GetMagY() const;

    const tc::Matrix4& GetMatrix() const;
    tc::Frustum GetFrustum() const;

protected:
    tc::Matrix4 CalcInfPerspective() const;
    tc::Matrix4 CalcPerspective() const;
    tc::Matrix4 CalcOrtho() const;

private:
    bool bIsOrthographic;
    float NearClip = 0.1f;
    float FarClip = 1000.0f;
    // Perspective only
    float AspectRatio = 1.0f;
    float FovY = 59.0f;
    // Ortho only
    float MagX = 1.0f;
    float MagY = 1.0f;

    mutable bool bMatrixValid = false;
    mutable tc::Matrix4 ProjectionMatrix;
};

} /* namespace Foreground */
