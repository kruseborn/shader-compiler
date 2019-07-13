#include "spirv-reflection.h"
#include <filesystem>
#include <set>
#include <tuple>

struct ShaderFile {
  std::string name, path, type;
  bool operator<(const ShaderFile &other) const {
    return std::tie(name, path, type) < std::tie(other.name, other.path, other.type);
  }
};

int main(int argc, char *argv[]) {
  if(argc != 2) {
    printf("spirv-reflection directory-path\n");
    return 1;
  }
  const std::string pathToSpirv = argv[1];
  std::set<ShaderFile> spirvShaders;

  for(auto & p : std::filesystem::directory_iterator(pathToSpirv)) {
    const std::string path = p.path().generic_string();
    std::string delimiter;
    std::string type;
    if(path.find("vert") != std::string::npos) {
      delimiter = ".vert";
      type = "glsl";
    }
    else if(path.find("frag") != std::string::npos) {
      delimiter = ".frag";
      type = "glsl";
    } else if (path.find("comp") != std::string::npos) {
      delimiter = ".comp";
      type = "comp";
    } else {
      assert(false);
    }
    const auto pathWithoutExtension = path.substr(0, path.find(delimiter));
    const auto name = pathWithoutExtension.substr(pathWithoutExtension.find_last_of("/") + 1);

    spirvShaders.insert({name, pathWithoutExtension, type});
  }

  std::vector<Shader> shaders;
  for(auto &spirvShader : spirvShaders) {
    if (spirvShader.type == "comp") {
      auto shader = parseComputeShader(spirvShader.name, spirvShader.path);
      shaders.push_back(shader);
    } else {
      auto shader = parseShader(spirvShader.name, spirvShader.path);
      shaders.push_back(shader);
    }
    
  }
  createCppStructs(shaders);

  return 0;
}

