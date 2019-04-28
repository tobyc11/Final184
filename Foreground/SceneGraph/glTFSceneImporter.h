#pragma once
#include "Scene.h"
#include <RHIInstance.h>

namespace Foreground
{

class CglTFSceneImporter
{
public:
    explicit CglTFSceneImporter(std::shared_ptr<CScene> scene, RHI::CDevice& device);

    void ImportFile(const std::string& path);

private:
    std::shared_ptr<CScene> Scene;
    RHI::CDevice& Device;
};

} /* namespace Foreground */
