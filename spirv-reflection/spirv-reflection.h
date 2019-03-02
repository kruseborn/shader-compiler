#pragma once

#define assert(x) do { if( !(x) ) { printf(#x); abort(); }} while(0)

#include <string>
#include <vector>

#define mat4 "glm::mat4"
#define mat3 "glm::mat3"
#define vec4 "glm::vec4"
#define vec3 "glm::vec3"
#define vec2 "glm::vec2"
#define ivec4 "glm::ivec4"
#define ivec3 "glm::ivec3"
#define ivec2 "glm::ivec2"

enum DescriptorSetTypes { UBO, COMBINED_IMAGE_SAMPLER };
static const char * descriptorSetTypesStr[] = {
  "UBO",
  "COMBINED_IMAGE_SAMPLER",
};
enum class InputFormats {
  FORMAT_R32_SFLOAT,
  FORMAT_R32G32_SFLOAT,
  FORMAT_R32G32B32_SFLOAT,
  FORMAT_R32G32B32A32_SFLOAT,
  FORMAT_R32_SINT,
  FORMAT_R32G32_SINT,
  FORMAT_R32G32B32_SINT,
  FORMAT_R32G32B32A32_SINT,
};

static const char * InputFormatsStr[] = {
  "VK_FORMAT_R32_SFLOAT",
  "VK_FORMAT_R32G32_SFLOAT",
  "VK_FORMAT_R32G32B32_SFLOAT",
  "VK_FORMAT_R32G32B32A32_SFLOAT",
  "VK_FORMAT_R32_SINT",
  "VK_FORMAT_R32G32_SINT",
  "VK_FORMAT_R32G32B32_SINT",
  "VK_FORMAT_R32G32B32A32_SINT",
};
static const char * InputFormatsTypeStr[] = {
  "float",
  vec2,
  vec3,
  vec4,
  "int32_t",
  ivec2,
  ivec3,
  ivec4,
};

struct UbuElement {
  std::string type;
  std::string name;
  uint32_t arraySize;
};

struct UboStruct {
  std::string name;
  std::vector<UbuElement> elements;
};

struct Ubo {
  std::string name;
  uint32_t sizeInBytes;
  std::vector<UbuElement> elements;
  std::vector<UboStruct> uboStructs;
};
using Ubos = std::vector<Ubo>;

struct VertexInput {
  std::string name;
  InputFormats format;
  uint32_t location, offset, binding, size;
};
using VertexInputs = std::vector<VertexInput>;

struct DescriptorSet {
  std::string name;
  uint32_t set, location, binding;
  DescriptorSetTypes descriptorSetType;
};
using DescriptorSets = std::vector<DescriptorSet>;

struct Shader {
  std::string name;
  Ubos ubos;
  VertexInputs vertexInputs;
  DescriptorSets descriptorSets;
  bool hasPushContant;
};

void createCppStructs(const std::vector<Shader> &shaders);
Shader parseShader(const std::string &name, const std::string &path);