#include "spirv-reflection.h"
#include <filesystem>
#include <map>
#include <tuple>

struct ShaderFile {
  std::string name, path, type;
  bool operator<(const ShaderFile &other) const {
    return std::tie(name, path, type) < std::tie(other.name, other.path, other.type);
  }
};

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("spirv-reflection directory-path\n");
    return 1;
  }
  const std::string pathToSpirv = argv[1];
  std::map<ShaderFile, std::vector<std::string>> spirvShaders;

  for (auto &p : std::filesystem::directory_iterator(pathToSpirv)) {
    const std::string path = p.path().generic_string();
    std::string delimiter;
    std::string type;
    if (path.find("vert") != std::string::npos) {
      delimiter = ".vert";
      type = "glsl";
    } else if (path.find("frag") != std::string::npos) {
      delimiter = ".frag";
      type = "glsl";
    } else if (path.find("comp") != std::string::npos) {
      delimiter = ".comp";
      type = "comp";
    } else if (path.find("rgen") != std::string::npos) {
      delimiter = ".rgen";
      type = "ray";
    } else if (path.find("rchit") != std::string::npos) {
      delimiter = ".rchit";
      type = "ray";
    } else if (path.find("rmiss") != std::string::npos) {
      delimiter = ".rmiss";
      type = "ray";
    } else if (path.find("rint") != std::string::npos) {
      delimiter = ".rint";
      type = "ray";
    } else {
      assert("shader type is not supported");
    }
    const auto pathWithoutExtension = path.substr(0, path.find("/"));
    auto fullName = path.substr(path.find_last_of("/") + 1);
    auto name = fullName.substr(0, fullName.find_first_of("."));

    spirvShaders[{name, pathWithoutExtension, type}].push_back(fullName);
  }

  std::vector<Shader> shaders;
  for (auto &spirvShader : spirvShaders) {
    auto shader = parseShader(spirvShader.first.name, spirvShader.first.path, spirvShader.second);
    shaders.push_back(shader);
  }
  createCppStructs(shaders);

  return 0;
}
