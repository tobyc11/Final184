#pragma once
#include <Matrix4.h>

namespace Foreground
{

class CRenderView
{
public:

private:
    tc::Matrix4 View;
    tc::Matrix4 Projection;

    // Perhaps more info for shadow mapping views?
};

} /* namespace Foreground */
