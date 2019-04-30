#include "ResourceManager.h"
#include <StringUtils.h>
#include <iostream>

#define STRINGIFY(sequence) #sequence
#define STRINGIFY_IND(sequence) STRINGIFY(sequence)
#define APP_SRC_DIR_STRING STRINGIFY_IND(APP_SOURCE_DIR)

CResourceManager& CResourceManager::Get()
{
    static CResourceManager singleton;
    return singleton;
}

void CResourceManager::Init()
{
    std::string appSrcDir = APP_SRC_DIR_STRING;
    appSrcDir = tc::FStringUtils::Trim(appSrcDir, '"');
    FilePath.insert(appSrcDir);

    std::string imguiFontsDir = appSrcDir + "/../imgui/misc/fonts";
    if (tc::FPathTools::Exists(imguiFontsDir))
    {
        FilePath.insert(tc::FPathTools::Compact(imguiFontsDir, '/'));
    }

    appSrcDir += "/Shader";
    ShaderPath.insert(appSrcDir);
    std::cout << "Using " << appSrcDir << " as the shader search directory." << std::endl;
}

void CResourceManager::Shutdown() {}

std::string CResourceManager::FindShader(const std::string& name)
{
    for (const auto& path : ShaderPath)
    {
        std::string result = path + "/" + name;
        bool exists = tc::FPathTools::Exists(result);
        if (exists)
            return result;
    }
    return std::string();
}

std::string CResourceManager::FindFile(const std::string& name)
{
    for (const auto& path : FilePath)
    {
        std::string result = path + "/" + name;
        bool exists = tc::FPathTools::Exists(result);
        if (exists)
            return result;
    }
    return std::string();
}
