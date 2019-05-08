#pragma once
#include <cstdint>
#include <string>

namespace Pl
{

// We used 64bit hashes as identifiers for efficiency reasons

class CHashId
{
public:
    CHashId();

private:
    uint64_t Value;
};

}
