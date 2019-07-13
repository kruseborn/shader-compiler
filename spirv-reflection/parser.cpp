#include "spirv-reflection.h"
#include <cctype>
#include <cstdio>
#include <fstream>
#include <spirv_glsl.hpp>

std::vector<uint32_t> readBinaryFromDisc(const std::string &fileName) {
  std::ifstream file(fileName, std::ios::binary | std::ios::ate);
  std::streamsize size = std::streamsize(file.tellg());
  file.seekg(0, std::ios::beg);

  std::vector<uint32_t> buffer(size / 4);
  if (file.read((char *)buffer.data(), size))
    return buffer;

  assert(false);
  return {};
}

std::string getBaseTypePrefix(spirv_cross::SPIRType::BaseType baseType) {
  std::string typePrefix;
  if (baseType == spirv_cross::SPIRType::BaseType::Float)
    typePrefix = "";
  else if (baseType == spirv_cross::SPIRType::BaseType::Double)
    typePrefix = "d";
  else if (baseType == spirv_cross::SPIRType::BaseType::Int)
    typePrefix = "i";
  else if (baseType == spirv_cross::SPIRType::BaseType::Boolean)
    typePrefix = "b";
  else
    assert(false);
  return typePrefix;
}

BufferElement parseElement(const spirv_cross::CompilerGLSL &compiler,
                           const spirv_cross::SPIRType &type, uint32_t index,
                           std::vector<BufferStruct> &uboStructs);

void parseStruct(const spirv_cross::CompilerGLSL &compiler,
                 const std::string &name, const spirv_cross::SPIRType &type,
                 std::vector<BufferStruct> &bufferStructs) {
  BufferStruct bufferStruct = {};
  bufferStruct.name = name;
  const auto member_count = type.member_types.size();
  if (member_count == 0) {
    parseElement(compiler, type, 0, bufferStructs);
  } else {
    for (uint32_t i = 0; i < member_count; i++) {
      auto element = parseElement(compiler, type, i, bufferStructs);
      bufferStruct.elements.push_back(element);
    }
  }
  bufferStructs.push_back(bufferStruct);
}

BufferElement parseElement(const spirv_cross::CompilerGLSL &compiler,
                           const spirv_cross::SPIRType &type, uint32_t index,
                           std::vector<BufferStruct> &bufferStructs) {
  BufferElement element;
  element.arraySize = 0;
  const auto &member_type = compiler.get_type(type.member_types[index]);

  if (type.basetype == spirv_cross::SPIRType::BaseType::Struct &&
      member_type.member_types.size() > 0) {
    size_t array_stride = compiler.type_struct_member_array_stride(type, index);

    auto structName = compiler.get_member_name(type.self, index);
    assert(!structName.empty());
    if (structName.back() == 's')
      structName.pop_back();
    structName[0] = std::toupper(structName[0]);
    parseStruct(compiler, structName, member_type, bufferStructs);
    element.type = structName;
  } else if (member_type.columns > 1) {
    element.type = getBaseTypePrefix(member_type.basetype);
    if (member_type.vecsize == 4 && member_type.columns == 4)
      element.type += mat4;
    else if (member_type.vecsize == 3 && member_type.columns == 4)
      element.type += mat3;
    else
      assert(false);
  } else if (member_type.vecsize > 1) {
    element.type = getBaseTypePrefix(member_type.basetype);
    if (member_type.vecsize == 4)
      element.type += vec4;
    else if (member_type.vecsize == 3)
      assert(
          false &&
          "do not use vec3, glsl and c/c++ does not have the same alignment");
    else if (member_type.vecsize == 2)
      element.type += vec2;
  } else {
    if (member_type.basetype == spirv_cross::SPIRType::BaseType::Float)
      element.type = "float";
    else if (member_type.basetype == spirv_cross::SPIRType::BaseType::Double)
      element.type = "double";
    else if (member_type.basetype == spirv_cross::SPIRType::BaseType::Int)
      element.type = "int32_t";
    else if (member_type.basetype == spirv_cross::SPIRType::BaseType::UInt)
      element.type = "uint32_t";
    else if (member_type.basetype == spirv_cross::SPIRType::BaseType::Boolean)
      element.type = "bool";
    else
      assert(false);
  }
  element.name = compiler.get_member_name(type.self, index);
  if (member_type.array.size() > 0) {
    if (member_type.array.front() > 0)
      element.name += "[" + std::to_string(member_type.array.front()) + "]";
    else {
      element.type += "*";
      element.name += " = nullptr";
    }
  }
  return element;
}

Ubos parseUbos(const spirv_cross::CompilerGLSL &compiler,
               const spirv_cross::ShaderResources &resources) {
  Ubos ubos = {};
  ubos.reserve(resources.uniform_buffers.size());

  for (const auto &resource : resources.uniform_buffers) {
    ShaderBuffer shaderBuffer = {};

    auto &type = compiler.get_type(resource.base_type_id);
    shaderBuffer.sizeInBytes =
        uint32_t(compiler.get_declared_struct_size(type));
    shaderBuffer.name = resource.name;

    const auto member_count = type.member_types.size();
    for (uint32_t i = 0; i < member_count; i++) {

      const auto element =
          parseElement(compiler, type, i, shaderBuffer.bufferStructs);
      shaderBuffer.elements.push_back(element);
      shaderBuffer.name = resource.name;
    }
    ubos.push_back(shaderBuffer);
  }

  return ubos;
}

SSBOs parseSSBOs(const spirv_cross::CompilerGLSL &compiler,
                 const spirv_cross::ShaderResources &resources) {
  SSBOs sSBOs = {};
  sSBOs.reserve(resources.storage_buffers.size());

  for (const auto &resource : resources.storage_buffers) {
    ShaderBuffer shaderBuffer = {};

    auto &type = compiler.get_type(resource.base_type_id);
    shaderBuffer.sizeInBytes =
        uint32_t(compiler.get_declared_struct_size(type));
    shaderBuffer.name = resource.name;

    const auto member_count = type.member_types.size();
    for (uint32_t i = 0; i < member_count; i++) {

      const auto element =
          parseElement(compiler, type, i, shaderBuffer.bufferStructs);
      shaderBuffer.elements.push_back(element);
      shaderBuffer.name = resource.name;
    }
    sSBOs.push_back(shaderBuffer);
  }

  return sSBOs;
}

bool parsePushConstant(const spirv_cross::CompilerGLSL &compiler,
                       const spirv_cross::ShaderResources &resources) {
  return resources.push_constant_buffers.size() > 0;
}

VertexInputs parseVertexInput(const spirv_cross::CompilerGLSL &compiler,
                              const spirv_cross::ShaderResources &resources) {
  VertexInputs vertexInputs = {};
  for (auto &resource : resources.stage_inputs) {
    VertexInput vertexInput = {};

    auto &type = compiler.get_type(resource.type_id);

    vertexInput.location = compiler.get_decoration(
        resource.id, spv::Decoration::DecorationLocation);
    vertexInput.binding = compiler.get_decoration(
        resource.id, spv::Decoration::DecorationBinding);

    assert(type.columns == 1);
    InputFormats format;

    if (type.basetype == spirv_cross::SPIRType::BaseType::Float) {
      switch (type.vecsize) {
      case 1:
        format = InputFormats::FORMAT_R32_SFLOAT;
        break;
      case 2:
        format = InputFormats::FORMAT_R32G32_SFLOAT;
        break;
      case 3:
        format = InputFormats::FORMAT_R32G32B32_SFLOAT;
        break;
      case 4:
        format = InputFormats::FORMAT_R32G32B32A32_SFLOAT;
        break;
      default:
        assert(false && "Input format not supported");
        break;
      }
      vertexInput.size = sizeof(float) * type.vecsize;
    } else if (type.basetype == spirv_cross::SPIRType::BaseType::Int) {
      switch (type.vecsize) {
      case 1:
        format = InputFormats::FORMAT_R32_SINT;
        break;
      case 2:
        format = InputFormats::FORMAT_R32G32_SINT;
        break;
      case 3:
        format = InputFormats::FORMAT_R32G32B32_SINT;
        break;
      case 4:
        format = InputFormats::FORMAT_R32G32B32A32_SINT;
        break;
      default:
        assert(false && "Input format not supported");
        break;
      }
      vertexInput.size = sizeof(int32_t) * type.vecsize;
    } else {
      assert(false && "Input format not supported");
    }
    vertexInput.format = format;
    vertexInput.name = compiler.get_name(resource.id);
    vertexInputs.push_back(vertexInput);
  }
  std::sort(
      std::begin(vertexInputs), std::end(vertexInputs),
      [](const auto &a, const auto &b) { return a.location < b.location; });

  for (uint32_t i = 1; i < uint32_t(vertexInputs.size()); i++) {
    vertexInputs[i].offset =
        vertexInputs[i - 1].size + vertexInputs[i - 1].offset;
  }
  return vertexInputs;
}

DescriptorSets
parseDescriptorSets(const spirv_cross::CompilerGLSL &compiler,
                    const spirv_cross::ShaderResources &resources) {
  DescriptorSets descriptorSets = {};

  for (auto &resource : resources.sampled_images) {
    DescriptorSet descriptorSet;
    descriptorSet.name = compiler.get_name(resource.id);
    descriptorSet.set = compiler.get_decoration(
        resource.id, spv::Decoration::DecorationDescriptorSet);
    descriptorSet.location = compiler.get_decoration(
        resource.id, spv::Decoration::DecorationLocation);
    descriptorSet.binding = compiler.get_decoration(
        resource.id, spv::Decoration::DecorationBinding);
    descriptorSet.descriptorSetType =
        DescriptorSetTypes::COMBINED_IMAGE_SAMPLER;
    descriptorSets.push_back(descriptorSet);
  }
  for (auto &resource : resources.uniform_buffers) {
    DescriptorSet descriptorSet;
    descriptorSet.name = compiler.get_name(resource.id);
    descriptorSet.set = compiler.get_decoration(
        resource.id, spv::Decoration::DecorationDescriptorSet);
    descriptorSet.location = compiler.get_decoration(
        resource.id, spv::Decoration::DecorationLocation);
    descriptorSet.binding = compiler.get_decoration(
        resource.id, spv::Decoration::DecorationBinding);
    descriptorSet.descriptorSetType = DescriptorSetTypes::UBO;
    descriptorSets.push_back(descriptorSet);
  }
  for (auto &resource : resources.storage_buffers) {
    DescriptorSet descriptorSet;
    descriptorSet.name = compiler.get_name(resource.id);
    descriptorSet.set = compiler.get_decoration(
        resource.id, spv::Decoration::DecorationDescriptorSet);
    descriptorSet.location = compiler.get_decoration(
        resource.id, spv::Decoration::DecorationLocation);
    descriptorSet.binding = compiler.get_decoration(
        resource.id, spv::Decoration::DecorationBinding);
    descriptorSet.descriptorSetType = DescriptorSetTypes::SSBO;
    descriptorSets.push_back(descriptorSet);
  }

  return descriptorSets;
}

Shader parseSpirv(const std::string &name, const std::string &path,
                  const std::string &type) {
  std::vector<uint32_t> spirv_binary = readBinaryFromDisc(path);
  spirv_cross::CompilerGLSL compiler(std::move(spirv_binary));

  spirv_cross::ShaderResources resources = compiler.get_shader_resources();

  Shader shader = {};
  shader.name = name;
  shader.ubos = parseUbos(compiler, resources);
  shader.ssbos = parseSSBOs(compiler, resources);
  if (type == "vert")
    shader.vertexInputs = parseVertexInput(compiler, resources);
  shader.descriptorSets = parseDescriptorSets(compiler, resources);
  std::stable_sort(
      std::begin(shader.descriptorSets), std::end(shader.descriptorSets),
      [](const auto &d1, const auto &d2) { return d1.binding < d2.binding; });
  shader.hasPushContant = parsePushConstant(compiler, resources);
  return shader;
}

Shader parseComputeShader(const std::string &name, const std::string &path) {
  auto computeShader = parseSpirv(name, path + ".comp.spv", "comp");
  return computeShader;
}

Shader parseShader(const std::string &name, const std::string &path) {
  auto vertexShader = parseSpirv(name, path + ".vert.spv", "vert");
  auto fragmentShader = parseSpirv(name, path + ".frag.spv", "frag");

  const auto fragSize = fragmentShader.descriptorSets.size();
  for (uint32_t i = 0; i < vertexShader.descriptorSets.size(); i++) {
    for (uint32_t j = 0; j < fragSize; j++) {
      auto d1 = vertexShader.descriptorSets[i];
      auto d2 = fragmentShader.descriptorSets[j];
      if (d1.name == d2.name) {
        assert(std::tie(d1.name, d1.set, d1.location) ==
               std::tie(d2.name, d2.set, d2.location));
        goto CONTINUE;
      }
    }
    fragmentShader.descriptorSets.push_back(vertexShader.descriptorSets[i]);
  CONTINUE:
    continue;
  }

  std::sort(std::begin(fragmentShader.descriptorSets),
            std::end(fragmentShader.descriptorSets),
            [](const auto &a, const auto &b) {
              return std::tie(a.set, a.location, a.binding) <
                     std::tie(b.set, b.location, b.binding);
            });
  vertexShader.descriptorSets = fragmentShader.descriptorSets;
  vertexShader.hasPushContant =
      vertexShader.hasPushContant || fragmentShader.hasPushContant;

  return vertexShader;
}
