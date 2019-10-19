#include "parsers.h"
#include "utils.h"
#include <cstring>

enum class State { common, vertex, fragment };

void parseRasterization(std::string fileName, std::ifstream fstream) {
  std::string vertex, fragment, common;
  vertex.reserve(1000);
  fragment.reserve(1000);
  common.reserve(1000);

  State state = State::common;
  std::string line;
  while (std::getline(fstream, line)) {
    if (line.find("@vert") != std::string::npos) {
      state = State::vertex;
      continue;
    } else if (line.find("@frag") != std::string::npos) {
      state = State::fragment;
      continue;
    }
    if (state == State::common) {
      common += line;
      common += "\n";
    } else if (state == State::vertex) {
      vertex += line;
      vertex += "\n";

    } else if (state == State::fragment) {
      fragment += line;
      fragment += "\n";
    } else {
      mgAssertDesc(false, "shader stage is not supported\n");
    }
  }
  mgAssertDesc(vertex.size(), "Could not parse vertex shader\n");
  mgAssertDesc(fragment.size(), "Could not parse fragment shader\n");

  common = parseInludes(common);
  vertex = parseInludes(vertex);
  fragment = parseInludes(fragment);

  const auto outVertex = common + vertex;
  const auto outFragment = common + fragment;

  char vf[50];
  char ff[50];
  removeSubstring(fileName, ".glsl");
  snprintf(vf, sizeof(vf), "%s.vert", fileName.c_str());
  snprintf(ff, sizeof(ff), "%s.frag", fileName.c_str());

  std::ofstream outVertexFile(vf, std::ofstream::out);
  std::ofstream outFragmentFile(ff, std::ofstream::out);
  mgAssertDesc(outVertexFile.is_open(), "could not create .vert file\n");
  mgAssertDesc(outFragmentFile.is_open(), "could not create .frag file\n");

  outVertexFile << outVertex;
  outFragmentFile << outFragment;

  outVertexFile.close();
  outFragmentFile.close();

  char vertexSpv[150];
  char fragmentSpv[150];
  const auto vulkanSdk = std::getenv("VULKAN_SDK");
  mgAssertDesc(strlen(vulkanSdk) > 0, "VULKAN_SDK is not set\n");
  snprintf(vertexSpv, sizeof(vertexSpv), "%s/bin/glslangValidator -V %s -o build/%s.spv", vulkanSdk, vf, vf);
  snprintf(fragmentSpv, sizeof(fragmentSpv), "%s/bin/glslangValidator -V %s -o build/%s.spv", vulkanSdk, ff, ff);

  printf("%s\n", fileName.c_str());
  auto errorString = getErrorAndWarnings(exec(vertexSpv));
  if (errorString.size()) {
    printShader(outVertex);
    printf("%s\n", errorString.c_str());
  }

  errorString = getErrorAndWarnings(exec(fragmentSpv));
  if (errorString.size()) {
    printShader(outFragment);
    printf("%s\n", errorString.c_str());
  }

  remove(vf);
  remove(ff);
}
