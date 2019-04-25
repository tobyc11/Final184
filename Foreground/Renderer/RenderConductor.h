#pragma once
#include "Primitive.h"
#include "RenderView.h"
#include <vector>

namespace Foreground
{

// Oversees the entire render process of a frame
//   Maybe subclassed/specialized to support different rendering algorithms
class CRenderConductor
{
public:
    void SetMainView(CRenderView* view);
    void SetMainDrawList(std::vector<CPrimitive*> drawList);

    void ProcessAndSubmit();
};

} /* namespace Foreground */
