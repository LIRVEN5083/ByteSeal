#include "shader_compile.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::vector<uint32_t> UTILS::CompileGLSLToSPIRV(const std::string& pathGLSL){
    std::ifstream file(pathGLSL);
    if (!file.is_open()) {
        std::cerr << "[SHADER COMPILER ERROR]: Could not open GLSL file: " << pathGLSL << "\n";
        return {};
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceCode = buffer.str();

    // Автоматически определяем тип шейдера
    shaderc_shader_kind shaderKind;
    if (pathGLSL.ends_with(".vert")) {
        shaderKind = shaderc_glsl_vertex_shader;
    } else if (pathGLSL.ends_with(".frag")) {
        shaderKind = shaderc_glsl_fragment_shader;
    } else if(pathGLSL.ends_with(".comp")){
        shaderKind = shaderc_glsl_compute_shader;
    } else {
        std::cerr << "[SHADER COMPILER ERROR]: Unknown shader type in file: " << pathGLSL << "\n";
        return {};
    }

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    shaderc::SpvCompilationResult compilationResult = compiler.CompileGlslToSpv(
        sourceCode, shaderKind, pathGLSL.c_str(), options
    );

    if (compilationResult.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << "\n[GLSL COMPILATION FAILED] in file: " << pathGLSL << "\n"
                  << compilationResult.GetErrorMessage() << "\n";
        return {};
    }

    return { compilationResult.cbegin(), compilationResult.cend() };
}
