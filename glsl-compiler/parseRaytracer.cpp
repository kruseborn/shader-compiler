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

static void createSprivFile(std::string fileName, const std::string &shader, State::Type type, const std::string &id) {
  char shaderFileName[50];
  removeSubstring(fileName, ".ray");
  std::string proc;
  if (type == State::procCHit || type == State::procInt) {
    proc = ".proc"; 
  }
  snprintf(shaderFileName, sizeof(shaderFileName), "%s%s%s%s", fileName.c_str(), id.c_str(), proc.c_str(), typeToExtension[type].c_str());

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
  constexpr uint32_t maxSameType = 5;
  std::string shaders[uint32_t(State::size)][maxSameType] = {};
  std::string shadersId[uint32_t(State::size)][maxSameType] = {};
  int32_t shaderIndex[uint32_t(State::size)] = {};
  memset(shaderIndex, -1, sizeof(shaderIndex));
  shaderIndex[State::common] = 0;
  for (uint32_t i = 0; i < State::size; i++) {
    shaders[i][0].reserve(1000);
  }

  State::Type state = State::common;
  std::string line;
  std::string id;
  while (std::getline(fstream, line)) {
    if (line.find("@gen") != std::string::npos) {
      state = State::gen;
      shaderIndex[state]++;
      id.clear();
      shadersId[state][shaderIndex[state]] = id;
      continue;
    } else if (line.find("@hit") != std::string::npos) {
      state = State::hit;
      shaderIndex[state]++;
      id.clear();
      shadersId[state][shaderIndex[state]] = id;
      continue;
    } else if (line.find("@miss") != std::string::npos) {
      state = State::miss;
      shaderIndex[state]++;
      id.clear();
      shadersId[state][shaderIndex[state]] = id;
      continue;
    } else if (line.find("@proc-int") != std::string::npos) {
      state = State::procInt;
      shaderIndex[state]++;
      id.clear();
      shadersId[state][shaderIndex[state]] = id;
      continue;
    } else if (line.find("@proc-chit") != std::string::npos) {
      state = State::procCHit;
      shaderIndex[state]++;
      id = "." + line.substr(strlen("@proc-chit "), std::string::npos);
      shadersId[state][shaderIndex[state]] = id;
      continue;
    }
    
    shaders[state][shaderIndex[state]] += line;
    shaders[state][shaderIndex[state]] += "\n";
  }
  printf("%s\n", fileName.c_str());

  // common = 0
  shaders[State::common][0] = parseInludes(shaders[State::common][0]);
  for (uint32_t i = 1; i < State::size; i++) {
    for (uint32_t j = 0; j < maxSameType; j++) {
      if (shaders[i][j].size() > 0) {
        shaders[i][j] = parseInludes(shaders[i][j]);
        createSprivFile(fileName, shaders[State::common][0] + shaders[i][j], State::Type(i), shadersId[i][j]);
      }
    }
  }
}
