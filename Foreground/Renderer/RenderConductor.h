#pragma once

#include "RenderView.h"
#include <SceneGraph/Primitive.h>
#include <vector>

namespace Foreground
{

// Oversees the entire render process of a frame
//   Maybe subclassed/specialized to support different rendering algorithms
class CRenderConductor
{
public:
    virtual void SetMainView(CRenderView* view) = 0;
    virtual void SetMainDrawList(std::vector<CPrimitive*> drawList) = 0;

    virtual void ProcessAndSubmit() = 0;
};

} /* namespace Foreground */
