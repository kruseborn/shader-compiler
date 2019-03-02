# Vulkan Shader Compiler
## glsl-compiler

Support for writing Vertex and Fragment shader in the same glsl file.

#### Prerequisite
* Enviroment variable VULKAN_SDK has to be set
* clang >= 7
* cmake >= 3.13
* Ninja >= 1.9

#### Build
```bash
mkdir build
cd build
cmake -G "Ninja" ..
cmake --build .
```
### Example

##### common.glsl
```glsl
vec4 redColor() {
    return vec4(1,0,0,1);
}
```

##### shader.glsl
```glsl
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "common.glsl"

struct Data {
    vec4 color;
    vec2 uv;
};

layout (set = 0, binding = 0) uniform UBO {
    vec2 scale;
    vec2 translate;
 } ubo;

@vert
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

out gl_PerVertex{
    vec4 gl_Position;
};

layout (location = 0) out Data outData;

void main()
{
    outData.color = inColor;
    outData.uv = inUV;
    gl_Position = vec4(inPos*ubo.scale+ubo.translate, 0, 1);
}

@frag
layout(location = 0) out vec4 outColor;
layout (location = 0) in Data inData;

layout(set=1, binding=0) uniform sampler2D sTexture;

void main() {
    // from common.glsl
    vec4 red = redColor();
    outColor = inData.color * texture(sTexture, inData.uv.st) * red;
}
```

#### Run
```bash
bin/glsl-compiler shader
```

## spirv-reflection
Spirv-reflection will parse a directory with spirv files and output shaderPipelineInput.h

#### Run
```bash
bin/spirv-reflection builds/
```

#### Output
```cpp
#pragma once

// This file is auto generated, don't touch
#include <glm/glm.hpp>
#include "vkContext.h"

namespace mg {
namespace shaders {

enum class Resources { UBO, COMBINED_IMAGE_SAMPLER };
struct VertexInputState {
  VkFormat format;
  uint32_t location, offset, binding, size;
};

namespace shader {
struct UBO {
  glm::vec2 scale;
  glm::vec2 translate;
};
namespace InputAssembler {
  static VertexInputState vertexInputState[3] = {
    { VK_FORMAT_R32G32_SFLOAT, 0, 0, 0, 8 },
    { VK_FORMAT_R32G32_SFLOAT, 1, 8, 0, 8 },
    { VK_FORMAT_R32G32B32A32_SFLOAT, 2, 16, 0, 16 },
  };
  struct VertexInputData {
    glm::vec2 inPos;
    glm::vec2 inUV;
    glm::vec4 inColor;
  };
};
namespace shaderResource {
  static bool hasPushConstant = false;
  static Resources resources[2] = {
    Resources::UBO,
    Resources::COMBINED_IMAGE_SAMPLER,
  };
  union DescriptorSets {
    struct {
      VkDescriptorSet ubo;
      VkDescriptorSet sTexture;
    };
    VkDescriptorSet values[2];
  };
  } // shaderResource
} //shader

} // shaders
} // mg
```
