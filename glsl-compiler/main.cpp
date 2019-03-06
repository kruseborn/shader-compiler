#include <fstream>
#include <string>
#include <sstream>
#include <memory>
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <filesystem>
#include <cctype>

void printShader(const std::string &shader) {
  uint32_t lineNr = 1;
  std::string line;
  std::stringstream myfile(shader);

  while(std::getline(myfile, line)) {
    printf("%d: %s\n", lineNr++, line.c_str());
  }
}

uint32_t nrOfLines(std::string in) {
  uint32_t number_of_lines = 0;
  std::string line;
  std::stringstream myfile(in);

  while(std::getline(myfile, line))
    ++number_of_lines;
  return number_of_lines;
}

// Try to find a Needle in a Haystack - ignore case
bool findString(const std::string & inStr, const std::string & pattern) {
  auto it = std::search(
    inStr.begin(), inStr.end(),
    pattern.begin(), pattern.end(),
    [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
  );
  return (it != inStr.end());
}
bool printErrorAndWarnings(const std::string strOutput) {
  bool foundError = false;
  std::istringstream vstream(strOutput);
  std::string vline;
  while(std::getline(vstream, vline)) {

    if(findString(vline, "warning") || findString(vline, "error")) {
      if(findString(vline, " is not yet complete")) // version warning
        continue;
      foundError = true;
      printf("%s\n", vline.c_str());
    }
  }
  return foundError;
}

std::string exec(const char* cmd) {
  char buffer[128];
  std::string result = "";
#ifdef _WIN32
  std::shared_ptr<FILE> pipe(_popen(cmd, "r"), _pclose);
#else
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);

#endif
  if(!pipe) assert(false);
  while(!feof(pipe.get())) {
    if(fgets(buffer, 128, pipe.get()) != NULL)
      result += buffer;
  }
  return result;
}

void _parseIncludes(std::string &in, std::string &out) {
  std::string line;
  std::stringstream stream(in);
  while(std::getline(stream, line)) {
    if(line.find("#include") != std::string::npos) {
      std::stringstream includeStream(line);

      std::string a, file;
      includeStream >> a >> file;
      file = file.substr(1, file.size() - 2);
      std::ifstream infile(file);
      if(!infile.good()) {
        printf("Could not open file: %s\n", file.c_str());
        exit(1);
      }
      std::string newStr;
      newStr.reserve(100);
      while(getline(infile, line)) {
        newStr += line;
        newStr += "\n";
      }
      _parseIncludes(newStr, out);

    }
    else {
      out += line;
      out += "\n";
    }
  }
}

bool exists(const std::string& name) {
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
  if(argc != 2) {
    printf("./glsl-compiler filename\n");
    return 1;
  }
  std::string inFile(argv[1]);
  std::ifstream infile(inFile + ".glsl");
  if(!infile.is_open()) {
    printf("Could not open file: %s.glsl\n", argv[1]);
    exit(1);
  }
  std::string vertex, fragment, common;
  vertex.reserve(1000);
  fragment.reserve(1000);
  common.reserve(1000);

  State state = State::common;
  std::string line;
  while(std::getline(infile, line)) {
    if(line.find("@vert") != std::string::npos) {
      state = State::vertex;
      continue;
    }
    else if(line.find("@frag") != std::string::npos) {
      state = State::fragment;
      continue;
    }
    if(state == State::common) {
      common += line;
      common += "\n";
    }
    else if(state == State::vertex) {
      vertex += line;
      vertex += "\n";

    }
    else if(state == State::fragment) {
      fragment += line;
      fragment += "\n";
    }
    else {
      assert(false && "shader stage is not supported");
    }
  }
  common = parseInludes(common);
  vertex = parseInludes(vertex);
  fragment = parseInludes(fragment);
  const auto outVertex = common + vertex;
  const auto outFragment = common + fragment;

  char vf[50];
  char ff[50];
  snprintf(vf, sizeof(vf), "%s.vert", inFile.c_str());
  snprintf(ff, sizeof(ff), "%s.frag", inFile.c_str());

  std::ofstream outVertexFile(vf, std::ofstream::out);
  std::ofstream outFragmentFile(ff, std::ofstream::out);
  assert(outVertexFile.is_open() && "could not create .vert file");
  assert(outFragmentFile.is_open() && "could not create .frag file");

  outVertexFile << outVertex;
  outFragmentFile << outFragment;

  outVertexFile.close();
  outFragmentFile.close();

  std::filesystem::create_directory("builds");
  
  char vertexSpv[150];
  char fragmentSpv[150];
  const auto vulkanSdk = std::getenv("VULKAN_SDK");
  snprintf(vertexSpv, sizeof(vertexSpv), "%s /bin/glslangValidator -V %s -o builds %s.spv", vulkanSdk, vf, vf);
  snprintf(fragmentSpv, sizeof(fragmentSpv), "%s /bin/glslangValidator -F %s -o builds %s.spv", vulkanSdk, ff, ff);

  bool foundError = false;

  printf("%s\n", inFile.c_str());
  foundError = printErrorAndWarnings(exec(vertexSpv));
  if(foundError)
    printShader(outVertex);

  foundError = printErrorAndWarnings(exec(fragmentSpv));
  if(foundError)
    printShader(outFragment);

  remove(vf);
  remove(ff);

  return 0;
}
