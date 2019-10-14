#include "parsers.h"
#include <cstdio>
#include <string>
#include <fstream>
#include <filesystem>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("./glsl-compiler filename\n");
    return 1;
  }
  std::string inFile(argv[1]);
  std::ifstream fstream(inFile);
  
  if (!fstream.is_open()) {
    printf("Could not open file: %s\n", argv[1]);
    exit(1);
  }
  const bool isComputeShader = inFile.find(".comp") != std::string::npos;
  const bool isRayTracing = inFile.find(".ray") != std::string::npos;
  const bool isRasterization = inFile.find(".glsl") != std::string::npos;

  std::filesystem::create_directory("build");

  if (isComputeShader) {
    parseCompute(inFile, std::move(fstream));
  } else if (isRayTracing) {
    parseRaytracer(inFile, std::move(fstream));
  } else if(isRasterization) {
    parseRasterization(inFile, std::move(fstream));
  }
  return 0;
}
