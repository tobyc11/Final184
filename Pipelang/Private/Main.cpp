#include "Pipelang.h"
#include <RHIInstance.h>

int main()
{
    auto device = RHI::CInstance::Get().CreateDevice(RHI::EDeviceCreateHints::NoHint);

    using namespace Pl;
    CPipelangContext context;
    context.SetDevice(device);
    auto& library = context.CreateLibrary("/Volumes/macData/BerkeleyCG/Final184/Pipelang/Internal");
    library.Parse();

    RHI::CPipelineDesc desc;
    library.GetPipeline(desc, {"EngineCommon", "PerPrimitive", "StandardTriMesh", "StaticMeshVS", "DefaultRasterizer", "NormalVisPS"});
    return 0;
}
