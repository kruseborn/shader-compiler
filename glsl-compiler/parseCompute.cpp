#include "parsers.h"
#include "utils.h"
#include <cstdio>

void parseCompute(const std::string &fileName, std::ifstream fstream) {
  char computeSpv[150];
  const auto vulkanSdk = std::getenv("VULKAN_SDK");
  mgAssertDesc(strlen(vulkanSdk) > 0, "VULKAN_SDK is not set\n");
  snprintf(computeSpv, sizeof(computeSpv),
           "%s/bin/glslangValidator -V %s -o build/%s.spv", vulkanSdk,
           fileName.c_str(), fileName.c_str());

  printf("%s\n", fileName.c_str());
  auto errorString = getErrorAndWarnings(exec(computeSpv));
  if (errorString.size()) {
    printShader(fileName.c_str());
    printf("%s\n", errorString.c_str());
  }
}
