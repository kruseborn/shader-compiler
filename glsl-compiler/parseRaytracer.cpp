#include "parsers.h"
#include "utils.h"
#include <unordered_map>

namespace State {
enum Type { common, gen, hit, miss, procInt, procCHit, size };
}
// clang-format off
std::unordered_map<State::Type, std::string> typeToExtension = {
    {State::gen, ".rgen"},
    {State::hit, ".rchit"},
    {State::miss, ".rmiss"},
    {State::procInt, ".rint"},
    {State::procCHit, ".rchit"}
};
// clang-format on

static void createSprivFile(std::string fileName, const std::string &shader, State::Type type) {
  char shaderFileName[50];
  removeSubstring(fileName, ".ray");
  snprintf(shaderFileName, sizeof(shaderFileName), "%s%s", fileName.c_str(), typeToExtension[type].c_str());

  std::ofstream fileStream(shaderFileName, std::ofstream::out);

  mgAssertDesc(fileStream.is_open(), "could not open file");

  fileStream << shader;
  fileStream.close();

  char spirv[150];
  const auto vulkanSdk = std::getenv("VULKAN_SDK");
  mgAssertDesc(strlen(vulkanSdk) > 0, "VULKAN_SDK is not set\n");
  snprintf(spirv, sizeof(spirv), "%s/bin/glslangValidator -V %s -o build/%s.spv", vulkanSdk, shaderFileName,
           shaderFileName);

  auto errorString = getErrorAndWarnings(exec(spirv));
  if (errorString.size()) {
    printShader(shader);
    printf("%s\n", errorString.c_str());
  }
  remove(shaderFileName);
}
void parseRaytracer(std::string fileName, std::ifstream fstream) {
  std::string shaders[uint32_t(State::size)] = {};
  for (uint32_t i = 0; i < State::size; i++) {
    shaders[i].reserve(1000);
  }

  State::Type state = State::common;
  std::string line;
  while (std::getline(fstream, line)) {
    if (line.find("@gen") != std::string::npos) {
      state = State::gen;
      continue;
    } else if (line.find("@hit") != std::string::npos) {
      state = State::hit;
      continue;
    } else if (line.find("@miss") != std::string::npos) {
      state = State::miss;
      continue;
    } else if (line.find("@proc-int") != std::string::npos) {
      state = State::procInt;
      continue;
    } else if (line.find("@proc-chit") != std::string::npos) {
      state = State::procCHit;
      continue;
    }
    shaders[state] += line;
    shaders[state] += "\n";
  }
  printf("%s\n", fileName.c_str());

  // common = 0
  shaders[State::common] = parseInludes(shaders[State::common]);
  for (uint32_t i = 1; i < State::size; i++) {
    if (shaders[i].size() > 0) {
      shaders[i] = parseInludes(shaders[i]);
      createSprivFile(fileName, shaders[State::common] + shaders[i], State::Type(i));
    }
  }
}
