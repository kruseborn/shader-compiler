#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#define mgAssertDesc(cond, msg)                                                \
  do {                                                                         \
    if (!(cond)) {                                                             \
      printf(msg);                                                         \
    }                                                                          \
  } while (0)
  

void removeSubstring(std::string& str, const std::string& substr) {
  std::string::size_type i = str.find(substr);

  if (i != std::string::npos)
    str.erase(i, substr.length());
}

void printShader(const std::string &shader) {
  uint32_t lineNr = 1;
  std::string line;
  std::stringstream myfile(shader);

  while (std::getline(myfile, line)) {
    printf("%d: %s\n", lineNr++, line.c_str());
  }
}

uint32_t nrOfLines(std::string in) {
  uint32_t number_of_lines = 0;
  std::string line;
  std::stringstream myfile(in);

  while (std::getline(myfile, line))
    ++number_of_lines;
  return number_of_lines;
}

// Try to find a Needle in a Haystack - ignore case
bool findString(const std::string &inStr, const std::string &pattern) {
  auto it = std::search(inStr.begin(), inStr.end(), pattern.begin(),
                        pattern.end(), [](char ch1, char ch2) {
                          return std::toupper(ch1) == std::toupper(ch2);
                        });
  return (it != inStr.end());
}
std::string getErrorAndWarnings(const std::string strOutput) {
  std::string errorString;
  std::istringstream vstream(strOutput);
  std::string vline;
  while (std::getline(vstream, vline)) {

    if (findString(vline, "warning") || findString(vline, "error")) {
      if (findString(vline, " is not yet complete")) // version warning
        continue;
      errorString += vline + "\n";
    }
  }
  return errorString;
}

std::string exec(const char *cmd) {
  char buffer[128];
  std::string result = "";
#ifdef _WIN32
  std::shared_ptr<FILE> pipe(_popen(cmd, "r"), _pclose);
#else
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);

#endif
  if (!pipe)
    mgAssertDesc(false, "could not open pipe\n");
  while (!feof(pipe.get())) {
    if (fgets(buffer, 128, pipe.get()) != NULL)
      result += buffer;
  }
  return result;
}

void _parseIncludes(std::string &in, std::string &out) {
  std::string line;
  std::stringstream stream(in);
  while (std::getline(stream, line)) {
    if (line.find("#include") != std::string::npos) {
      std::stringstream includeStream(line);

      std::string a, file;
      includeStream >> a >> file;
      file = file.substr(1, file.size() - 2);
      std::ifstream infile(file);
      if (!infile.good()) {
        printf("Could not open file: %s\n", file.c_str());
        exit(1);
      }
      std::string newStr;
      newStr.reserve(100);
      while (getline(infile, line)) {
        newStr += line;
        newStr += "\n";
      }
      _parseIncludes(newStr, out);

    } else {
      out += line;
      out += "\n";
    }
  }
}

bool exists(const std::string &name) {
  std::ifstream f(name.c_str());
  return f.good();
}

std::string parseInludes(std::string &in) {
  std::string out;
  out.reserve(500);
  _parseIncludes(in, out);
  return out;
}

enum class State { common, vertex, fragment };

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("./glsl-compiler filename\n");
    return 1;
  }
  std::string inFile(argv[1]);
  std::ifstream infile(inFile);
  
  if (!infile.is_open()) {
    printf("Could not open file: %s\n", argv[1]);
    exit(1);
  }
  const bool isComputeShader = inFile.find(".comp") != std::string::npos;

  std::string vertex, fragment, common;
  vertex.reserve(1000);
  fragment.reserve(1000);
  common.reserve(1000);

  State state = State::common;
  std::string line;
  while (std::getline(infile, line)) {
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
  if (!isComputeShader) {
    mgAssertDesc(vertex.size(), "Could not parse vertex shader\n");
    mgAssertDesc(fragment.size(), "Could not parse fragment shader\n");

    common = parseInludes(common);
    vertex = parseInludes(vertex);  
    fragment = parseInludes(fragment);

    const auto outVertex = common + vertex;
    const auto outFragment = common + fragment;

    char vf[50];
    char ff[50];
    removeSubstring(inFile, ".glsl");
    snprintf(vf, sizeof(vf), "%s.vert", inFile.c_str());
    snprintf(ff, sizeof(ff), "%s.frag", inFile.c_str());

    std::ofstream outVertexFile(vf, std::ofstream::out);
    std::ofstream outFragmentFile(ff, std::ofstream::out);
    mgAssertDesc(outVertexFile.is_open(), "could not create .vert file\n");
    mgAssertDesc(outFragmentFile.is_open(), "could not create .frag file\n");

    outVertexFile << outVertex;
    outFragmentFile << outFragment;

    outVertexFile.close();
    outFragmentFile.close();

    std::filesystem::create_directory("build");

    char vertexSpv[150];
    char fragmentSpv[150];
    const auto vulkanSdk = std::getenv("VULKAN_SDK");
    mgAssertDesc(strlen(vulkanSdk) > 0, "VULKAN_SDK is not set\n");
    snprintf(vertexSpv, sizeof(vertexSpv),
             "%s/bin/glslangValidator -V %s -o build/%s.spv", vulkanSdk, vf,
             vf);
    snprintf(fragmentSpv, sizeof(fragmentSpv),
             "%s/bin/glslangValidator -V %s -o build/%s.spv", vulkanSdk, ff,
             ff);

    printf("%s\n", inFile.c_str());
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
  } else {
    std::filesystem::create_directory("build");

    char computeSpv[150];
    const auto vulkanSdk = std::getenv("VULKAN_SDK");
    mgAssertDesc(strlen(vulkanSdk) > 0, "VULKAN_SDK is not set\n");
    snprintf(computeSpv, sizeof(computeSpv),
             "%s/bin/glslangValidator -V %s -o build/%s.spv", vulkanSdk, inFile.c_str(),
             inFile.c_str());

    printf("%s\n", inFile.c_str());
    auto errorString = getErrorAndWarnings(exec(computeSpv));
    if (errorString.size()) {
      printShader(common);
      printf("%s\n", errorString.c_str());
    }
  }

  return 0;
}
