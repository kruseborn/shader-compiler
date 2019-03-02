#include "spirv-reflection.h"

#include <cstdio>

const std::string mathPath = "glm/glm.hpp";
const std::string vulkanPath = "vkContext.h";

void createHeader(FILE *file) {
  fprintf(file, "#pragma once\n");
  fprintf(file, "\n");
  fprintf(file, "// This file is auto generated, don�t touch\n");
  fprintf(file, "\n");
  fprintf(file, "#include <%s>\n", mathPath.c_str());
  fprintf(file, "#include \"%s\"\n", vulkanPath.c_str());
  fprintf(file, "\n");
  fprintf(file, "namespace mg {\n");
  fprintf(file, "namespace shaders {\n");
  fprintf(file, "\n");
  fprintf(file, "enum class Resources { UBO, COMBINED_IMAGE_SAMPLER };\n");
  fprintf(file, "struct VertexInputState {\n");
  fprintf(file, "  VkFormat format;\n");
  fprintf(file, "  uint32_t location, offset, binding, size;\n");
  fprintf(file, "};\n");
}

void createFooter(FILE *file) {
  fprintf(file, "\n");
  fprintf(file, "} // shaders\n");
  fprintf(file, "} // mg\n");
}

void createShaderHeader(FILE *file, const Shader &shader) {
  fprintf(file, "\nnamespace %s {\n", shader.name.c_str());
}

void createShaderFooter(FILE *file, const Shader &shader) {
  fprintf(file, "} //%s\n", shader.name.c_str());
}

void createInternalStruct(FILE *file, std::vector<UboStruct> uboStructs) {
  for(int32_t i = int32_t(uboStructs.size()) - 1; i >= 0; i--) {
    fprintf(file, "  struct %s {\n", uboStructs[i].name.c_str());
    for(uint32_t j = 0; j < uint32_t(uboStructs[i].elements.size()); j++) {
      const auto &element = uboStructs[i].elements[j];
      fprintf(file, "    %s %s;\n", element.type.c_str(), element.name.c_str());
    }
    fprintf(file, "  };\n");
  }
}

void createUboStruct(FILE *file, const Shader &shader) {
  for(uint32_t i = 0; i < uint32_t(shader.ubos.size()); i++) {
    fprintf(file, "struct %s {\n", shader.ubos[i].name.c_str());

    createInternalStruct(file, shader.ubos[i].uboStructs);
    for(uint32_t j = 0; j < uint32_t(shader.ubos[i].elements.size()); j++) {
      const auto &element = shader.ubos[i].elements[j];
      fprintf(file, "  %s %s;\n", element.type.c_str(), element.name.c_str());
    }
    fprintf(file, "};\n");
  }
}

void createInputAssembler(FILE *file, const Shader &shader) {
  fprintf(file, "namespace InputAssembler {\n");
  fprintf(file, "  static VertexInputState vertexInputState[%u] = {\n", uint32_t(shader.vertexInputs.size()));
  for(uint32_t i = 0; i < uint32_t(shader.vertexInputs.size()); i++) {
    const auto &vertexInput = shader.vertexInputs[i];
    fprintf(file, "    { %s, %d, %d, %d, %d },\n", InputFormatsStr[uint32_t(vertexInput.format)], vertexInput.location, vertexInput.offset, vertexInput.binding, vertexInput.size);
  }
  fprintf(file, "  };\n");
  fprintf(file, "  struct VertexInputData {\n");
  for(uint32_t i = 0; i < uint32_t(shader.vertexInputs.size()); i++) {
    const auto &vertexInput = shader.vertexInputs[i];
    fprintf(file, "    %s %s;\n", InputFormatsTypeStr[uint32_t(vertexInput.format)], vertexInput.name.c_str());
  }
  fprintf(file, "  };\n");
  fprintf(file, "};\n");
}

void createDescriptorSets(FILE *file, const Shader &shader) {
  fprintf(file, "namespace shaderResource {\n");
  fprintf(file, "  static bool hasPushConstant = %s;\n", shader.hasPushContant ? "true": "false");
  fprintf(file, "  static Resources resources[%u] = {\n", uint32_t(shader.descriptorSets.size()));
  for(uint32_t i = 0; i < uint32_t(shader.descriptorSets.size()); i++) {
    fprintf(file, "    Resources::%s,\n", descriptorSetTypesStr[shader.descriptorSets[i].descriptorSetType]);
  }
  fprintf(file, "  };\n");
  fprintf(file, "  union DescriptorSets {\n");
  fprintf(file, "    struct {\n");
  for(uint32_t i = 0; i < uint32_t(shader.descriptorSets.size()); i++) {
    const auto &descriptorSet = shader.descriptorSets[i];
    fprintf(file, "      VkDescriptorSet %s;\n", descriptorSet.name.c_str());
  }
  fprintf(file, "    };\n");
  fprintf(file, "    VkDescriptorSet values[%u];\n", uint32_t(shader.descriptorSets.size()));
  fprintf(file, "  };\n");
  fprintf(file, "  } // shaderResource\n");
}

void createCppStructs(const std::vector<Shader> &shaders) {
  FILE *file = fopen("shaderPipelineInput.h", "w");
  createHeader(file);
  for(const auto &shader : shaders) {
    createShaderHeader(file, shader);
    if(shader.ubos.size() > 0) createUboStruct(file, shader);
    if(shader.vertexInputs.size() > 0) createInputAssembler(file, shader);
    if(shader.descriptorSets.size() > 0) createDescriptorSets(file, shader);
    createShaderFooter(file, shader);
  }
  createFooter(file);
  fclose(file);
}