#include "Camera.h"

#include <cmath>

namespace Foreground
{

CCamera::CCamera(bool ortho)
    : bIsOrthographic(ortho)
{
}

void CCamera::SetAspectRatio(float value)
{
    AspectRatio = value;
    bMatrixValid = false;
}

float CCamera::GetAspectRatio() const { return AspectRatio; }

void CCamera::SetFovY(float value)
{
    FovY = value;
    bMatrixValid = false;
}

float CCamera::GetFovY() const { return FovY; }

void CCamera::SetNearClip(float value)
{
    NearClip = value;
    bMatrixValid = false;
}

void CCamera::SetFarClip(float value)
{
    FarClip = value;
    bMatrixValid = false;
}

float CCamera::GetNearClip() const { return NearClip; }

float CCamera::GetFarClip() const { return FarClip; }

void CCamera::SetMagX(float value)
{
    MagX = value;
    bMatrixValid = false;
}

void CCamera::SetMagY(float value)
{
    MagY = value;
    bMatrixValid = false;
}

float CCamera::GetMagX() const { return MagX; }

float CCamera::GetMagY() const { return MagY; }

const tc::Matrix4& CCamera::GetMatrix() const
{
    if (!bMatrixValid)
        if (bIsOrthographic)
            ProjectionMatrix = CalcOrtho();
        else if (FarClip >= 100000.f)
            ProjectionMatrix = CalcInfPerspective();
        else
            ProjectionMatrix = CalcPerspective();
    bMatrixValid = true;
    return ProjectionMatrix;
}

tc::Frustum CCamera::GetFrustum() const
{
    tc::Frustum frustum;
    frustum.Define(GetMatrix());
    return frustum;
}

tc::Matrix4 CCamera::CalcInfPerspective() const { return tc::Matrix4(); }

tc::Matrix4 CCamera::CalcPerspective() const
{
    float rad_fovy = FovY / 180.f * tc::M_PI;
    float y_slope = tan(rad_fovy);
    float x_slope = y_slope * AspectRatio;
    // aspect ratio = w / h   =>   w = aspect ratio * h

    float right = x_slope * NearClip;
    float top = y_slope * NearClip;

    float a = NearClip / right;
    float b = NearClip / top;
    float c = (FarClip + NearClip) / (NearClip - FarClip);
    float d = -2 * FarClip * NearClip / (FarClip - NearClip);

    // Negate b since Vulkan uses a flipped NDC
    return tc::Matrix4(a, 0, 0, 0, 0, -b, 0, 0, 0, 0, c, d, 0, 0, -1, 0);
}

tc::Matrix4 CCamera::CalcOrtho() const { return tc::Matrix4(); }

} /* namespace Foreground */
