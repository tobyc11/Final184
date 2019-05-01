#pragma once
#include "ForegroundCommon.h"
#include <Resources.h>

namespace Foreground
{

class CMegaPipeline
{
public:
    CMegaPipeline(RHI::CSwapChain::Ref swapChain);

protected:
    void CreateRenderPasses();
    void CreateGBufferPass(uint32_t width, uint32_t height);
    void CreateScreenPass();

private:
    RHI::CSwapChain::Ref SwapChain;

    RHI::CImageView::Ref GBuffer0;
    RHI::CImageView::Ref GBuffer1;
    RHI::CImageView::Ref GBuffer2;
    RHI::CImageView::Ref GBuffer3;
    RHI::CImageView::Ref GBufferDepth;
    RHI::CRenderPass::Ref GBufferPass;

    RHI::CImageView::Ref FBView;
    RHI::CRenderPass::Ref ScreenPass;
};

} /* namespace Foreground */
