#include "spirv-reflection.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <spirv_glsl.hpp>

template <class A, class B> void insert(A &a, B &b) { a.insert(std::end(a), std::begin(b), std::end(b)); }

static std::vector<uint32_t> readBinaryFromDisc(const std::string &fileName) {
  std::ifstream file(fileName, std::ios::binary | std::ios::ate);
  std::streamsize size = std::streamsize(file.tellg());
  file.seekg(0, std::ios::beg);

  std::vector<uint32_t> buffer(size / 4);
  if (file.read((char *)buffer.data(), size))
    return buffer;

  assert(false);
  return {};
}

static std::string getBaseTypePrefix(spirv_cross::SPIRType::BaseType baseType) {
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

static BufferElement parseElement(const spirv_cross::CompilerGLSL &compiler, const spirv_cross::SPIRType &type,
                                  uint32_t index, std::vector<BufferStruct> &uboStructs);

static void parseStruct(const spirv_cross::CompilerGLSL &compiler, const std::string &name,
                        const spirv_cross::SPIRType &type, std::vector<BufferStruct> &bufferStructs) {
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

static BufferElement parseElement(const spirv_cross::CompilerGLSL &compiler, const spirv_cross::SPIRType &type,
                                  uint32_t index, std::vector<BufferStruct> &bufferStructs) {
  BufferElement element;
  element.arraySize = 0;
  const auto &member_type = compiler.get_type(type.member_types[index]);

  if (type.basetype == spirv_cross::SPIRType::BaseType::Struct && member_type.member_types.size() > 0) {
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
      assert(false && "do not use vec3, glsl and c/c++ does not have the same alignment");
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

static ShaderBuffer parseResource(const spirv_cross::CompilerGLSL &compiler, const spirv_cross::Resource &resource) {
  ShaderBuffer shaderBuffer = {};

  auto &type = compiler.get_type(resource.base_type_id);
  shaderBuffer.sizeInBytes = uint32_t(compiler.get_declared_struct_size(type));
  shaderBuffer.name = compiler.get_name(resource.base_type_id);

  const auto member_count = type.member_types.size();
  for (uint32_t i = 0; i < member_count; i++) {

    const auto element = parseElement(compiler, type, i, shaderBuffer.bufferStructs);
    shaderBuffer.elements.push_back(element);
  }
  return shaderBuffer;
}

Ubos parseUbos(const spirv_cross::CompilerGLSL &compiler, const spirv_cross::ShaderResources &resources) {
  Ubos ubos = {};
  ubos.reserve(resources.uniform_buffers.size());

  for (const auto &resource : resources.uniform_buffers) {
    ubos.push_back(parseResource(compiler, resource));
  }

  return ubos;
}

PushConstants parseSSBOs(const spirv_cross::CompilerGLSL &compiler, const spirv_cross::ShaderResources &resources) {
  SSBOs sSBOs = {};
  sSBOs.reserve(resources.storage_buffers.size());

  for (const auto &resource : resources.storage_buffers) {
    sSBOs.push_back(parseResource(compiler, resource));
  }
  return sSBOs;
}

PushConstants parsePushConstant(const spirv_cross::CompilerGLSL &compiler,
                                const spirv_cross::ShaderResources &resources) {
  PushConstants pushContants = {};
  pushContants.reserve(resources.push_constant_buffers.size());

  for (const auto &resource : resources.push_constant_buffers) {
    pushContants.push_back(parseResource(compiler, resource));
  }
  return pushContants;
}

VertexInputs parseVertexInput(const spirv_cross::CompilerGLSL &compiler,
                              const spirv_cross::ShaderResources &resources) {
  VertexInputs vertexInputs = {};
  for (auto &resource : resources.stage_inputs) {
    VertexInput vertexInput = {};

    auto &type = compiler.get_type(resource.type_id);

    vertexInput.location = compiler.get_decoration(resource.id, spv::Decoration::DecorationLocation);
    vertexInput.binding = compiler.get_decoration(resource.id, spv::Decoration::DecorationBinding);

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
  std::sort(std::begin(vertexInputs), std::end(vertexInputs),
            [](const auto &a, const auto &b) { return a.location < b.location; });

  for (uint32_t i = 1; i < uint32_t(vertexInputs.size()); i++) {
    vertexInputs[i].offset = vertexInputs[i - 1].size + vertexInputs[i - 1].offset;
  }
  return vertexInputs;
}

static DescriptorSet parseDescriptorSet(const spirv_cross::CompilerGLSL &compiler,
                                        const spirv_cross::Resource &resource) {
  DescriptorSet descriptorSet = {};
  descriptorSet.name = compiler.get_name(resource.id);
  descriptorSet.set = compiler.get_decoration(resource.id, spv::Decoration::DecorationDescriptorSet);
  descriptorSet.location = compiler.get_decoration(resource.id, spv::Decoration::DecorationLocation);
  descriptorSet.binding = compiler.get_decoration(resource.id, spv::Decoration::DecorationBinding);
  return descriptorSet;
}

DescriptorSets parseDescriptorSets(const spirv_cross::CompilerGLSL &compiler,
                                   const spirv_cross::ShaderResources &resources) {
  DescriptorSets descriptorSets = {};

  for (auto &resource : resources.sampled_images) {
    DescriptorSet descriptorSet = parseDescriptorSet(compiler, resource);
    descriptorSets.push_back(descriptorSet);
  }
  for (auto &resource : resources.separate_images) {
    DescriptorSet descriptorSet = parseDescriptorSet(compiler, resource);
    descriptorSets.push_back(descriptorSet);
  }
  for (auto &resource : resources.uniform_buffers) {
    DescriptorSet descriptorSet = parseDescriptorSet(compiler, resource);
    descriptorSets.push_back(descriptorSet);
  }
  for (auto &resource : resources.storage_buffers) {
    DescriptorSet descriptorSet = parseDescriptorSet(compiler, resource);
    descriptorSets.push_back(descriptorSet);
  }
  for (auto &resource : resources.acceleration_structures) {
    DescriptorSet descriptorSet = parseDescriptorSet(compiler, resource);
    descriptorSets.push_back(descriptorSet);
  }
  for (auto &resource : resources.storage_images) {
    DescriptorSet descriptorSet = parseDescriptorSet(compiler, resource);
    descriptorSets.push_back(descriptorSet);
  }
  return descriptorSets;
}

void parseSpirv(Shader *shader, const std::string &path, const std::string &type) {

  std::vector<uint32_t> spirv_binary = readBinaryFromDisc(path);
  spirv_cross::CompilerGLSL compiler(std::move(spirv_binary));

  spirv_cross::ShaderResources resources = compiler.get_shader_resources();

  insert(shader->ubos, parseUbos(compiler, resources));
  insert(shader->ssbos, parseSSBOs(compiler, resources));
  insert(shader->pushConstants, parsePushConstant(compiler, resources));
  if (type == "vert") {
    insert(shader->vertexInputs, parseVertexInput(compiler, resources));
  }
  insert(shader->descriptorSets, parseDescriptorSets(compiler, resources));
}

template <class T, class Func> void makeUnqiue(T &t, Func &&func) {
  auto it = std::unique(std::begin(t), std::end(t), func);
  t.resize(std::distance(std::begin(t), it));
}

void sortAndMakeUnqiue(Shader *shader) {
  std::stable_sort(std::begin(shader->descriptorSets), std::end(shader->descriptorSets),
                   [](const auto &a, const auto &b) {
                     return std::tie(a.set, a.location, a.binding) < std::tie(b.set, b.location, b.binding);
                   });

  std::stable_sort(std::begin(shader->ubos), std::end(shader->ubos),
                   [](const auto &a, const auto &b) { return a.name < b.name; });
  std::stable_sort(std::begin(shader->ssbos), std::end(shader->ssbos),
                   [](const auto &a, const auto &b) { return a.name < b.name; });
  std::stable_sort(std::begin(shader->pushConstants), std::end(shader->pushConstants),
                   [](const auto &a, const auto &b) { return a.name < b.name; });

  makeUnqiue(shader->descriptorSets, [](auto &a, auto &b) { return a.set == b.set; });
  makeUnqiue(shader->ubos, [](auto &a, auto &b) { return a.name == b.name; });
  makeUnqiue(shader->ssbos, [](auto &a, auto &b) { return a.name == b.name; });
  makeUnqiue(shader->pushConstants, [](auto &a, auto &b) { return a.name == b.name; });
}

Shader parseComputeShader(const std::string &name, const std::string &path) {
  Shader shader = {};
  shader.name = name;

  parseSpirv(&shader, path + ".comp.spv", "comp");
  sortAndMakeUnqiue(&shader);
  return shader;
}

Shader parseRasterizationShader(const std::string &name, const std::string &path) {
  Shader shader = {};
  shader.name = name;

  parseSpirv(&shader, path + ".vert.spv", "vert");
  parseSpirv(&shader, path + ".frag.spv", "frag");

  sortAndMakeUnqiue(&shader);

  return shader;
}

static inline bool exists(const std::string &name) {
  if (FILE *file = fopen(name.c_str(), "r")) {
    fclose(file);
    return true;
  } else {
    return false;
  }
}

Shader parseRayTracingShader(const std::string &name, const std::string &path) {
  Shader shader = {};
  shader.name = name;

  parseSpirv(&shader, path + ".rgen.spv", "");
  parseSpirv(&shader, path + ".rchit.spv", "");
  parseSpirv(&shader, path + ".rmiss.spv", "");

  if (exists(path + ".rint.spv")) {
    parseSpirv(&shader, path + ".rint.spv", "");
  }

  sortAndMakeUnqiue(&shader);
  return shader;
}
