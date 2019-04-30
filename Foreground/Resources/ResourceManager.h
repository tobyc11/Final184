#pragma once
#include <PathTools.h>
#include <set>
#include <string>

class CResourceManager
{
public:
    static CResourceManager& Get();

    void Init();
    void Shutdown();
    std::string FindShader(const std::string& name);
    std::string FindFile(const std::string& name);

private:
    std::set<std::string> ShaderPath;
    std::set<std::string> FilePath;
};
