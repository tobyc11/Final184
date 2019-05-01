#pragma once
#include <Device.h>
#include <TypeId.h>
#include <memory>
#include <unordered_map>

namespace Foreground
{

// Main mechanism to track shaders
class CShaderFactory
{
public:
    CShaderFactory();

    template <typename T> std::shared_ptr<T> GetShader()
    {
        size_t id = tc::type_id<T>();
        auto iter = Trackers.find(id);
        if (iter != Trackers.end() && !iter->second.expired())
            return std::static_pointer_cast<T>(iter->second.lock());

        // Otherwise create new instance of that shader
        auto shader = std::make_shared<T>();
        Trackers[id] = shader;
        return shader;
    }

private:
    // RHI::CDevice::Ref Device;
    std::unordered_map<size_t, std::weak_ptr<void>> Trackers;
};

}
