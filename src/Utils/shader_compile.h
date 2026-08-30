#pragma once
#include <shaderc/shaderc.hpp>

namespace UTILS{
    std::vector<uint32_t> CompileGLSLToSPIRV(const std::string& pathGLSL);
}