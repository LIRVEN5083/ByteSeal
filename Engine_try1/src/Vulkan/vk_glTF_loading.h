#pragma once

#include "vk_types.h"

#if defined(_WIN32) && defined(ERROR) && defined(TINYGLTF_ENABLE_DRACO)
#undef ERROR
#pragma message ("ERROR constant already defined, undefining")
#endif

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tinygltf/tiny_gltf.h"

// Макс количество костей. (Их изменение заставит поменять вершинные шейдеры)
#define MAX_NUM_JOINTS 128u

namespace VK_LOADING{

}