#include "Camera.h"

#include <cmath>

namespace Foreground
{

CCamera::CCamera(bool ortho): bIsOrthographic(ortho) {
    assert(!ortho); // unimplemented
}

tc::Matrix4 CCamera::CalcPerspective() {
    assert(!bIsOrthographic);

    if (!bMatrixValid) {
        double rad_fovy = FovY / 180 * tc::M_PI;
        double y_slope = tan(rad_fovy);
        double x_slope = y_slope * AspectRatio; 
        // aspect ratio = w / h   =>   w = aspect ratio * h

        double right = x_slope * NearClip;
        double top = y_slope * NearClip;

        double a = NearClip / right;
        double b = NearClip / top;
        double c = (FarClip + NearClip) / (NearClip - FarClip);
        double d = -2 * FarClip * NearClip / (FarClip - NearClip);

        bMatrixValid = true;
        ProjectionMatrix = tc::Matrix4(
                a,  0,  0,  0,
                0,  b,  0,  0,
                0,  0,  c,  d,
                0,  0, -1,  0
                );
    }

    return ProjectionMatrix;
}

void CCamera::SetAspectRatio(float value) {
    bMatrixValid = false;
    AspectRatio = value;
}
float CCamera::GetAspectRatio() const {
    return AspectRatio;
}

void CCamera::SetFovY(float value) {
    bMatrixValid = false;
    FovY = value;
}
float CCamera::GetFovY() const {
    return FovY;
}

void CCamera::SetNearClip(float value) {
    bMatrixValid = false;
    NearClip = value;
}
float CCamera::GetNearClip() const {
    return NearClip;
}

void CCamera::SetFarClip(float value) {
    bMatrixValid = false;
    FarClip = value;
}
float CCamera::GetFarClip() const {
    return FarClip;
}


void CCamera::SetMagX(float value) {
    bMatrixValid = false;
    MagX = value;
}
float CCamera::GetMagX() const {
    return MagX;
}

void CCamera::SetMagY(float value) {
    bMatrixValid = false;
    MagY = value;
}
float CCamera::GetMagY() const {
    return MagY;
}

} /* namespace Foreground */
