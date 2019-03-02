#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <memory>
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>

void printShader(const std::string &shader) {
  int lineNr = 1;
  std::string line;
  std::stringstream myfile(shader);

  while(std::getline(myfile, line))
    std::cout << lineNr++ << ": " << line << std::endl;
}

int nrOfLines(std::string in) {
  int number_of_lines = 0;
  std::string line;
  std::stringstream myfile(in);

  while(std::getline(myfile, line))
    ++number_of_lines;
  return number_of_lines;
}

/// Try to find in the Haystack the Needle - ignore case
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
      std::cout << vline << std::endl;
    }
  }
  return foundError;
}

std::string exec(const char* cmd) {
  char buffer[128];
  std::string result = "";
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  
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
        std::cout << "could not open file: " << file << std::endl;
        exit(1);
      }
      std::string newStr;
      while(getline(infile, line))
        newStr += line + "\n";
      _parseIncludes(newStr, out);

    }
    else
      out += line + "\n";
  }
}

bool exists(const std::string& name) {
  std::ifstream f(name.c_str());
  return f.good();
}

std::string parseInludes(std::string &in) {
  std::string out;
  _parseIncludes(in, out);
  return out;
}

enum class State { common, vertex, fragment };

int main(int argc, char *argv[]) {
  if(argc != 2) {
    std::cout << "shaderCompiler filename " << std::endl;
    return 1;
  }
  std::string inFile(argv[1]);
  std::ifstream infile(inFile + ".glsl");
  if(!infile.is_open()) {
    std::cout << "Could not open file: " << argv[1] << ".glsl" << std::endl;
    exit(1);
  }
  std::string line;
  std::string vertex, fragment, common;
  State state = State::common;
  while(std::getline(infile, line)) {
    if(line.find("@vert") != std::string::npos) {
      state = State::vertex;
      continue;
    }
    else if(line.find("@frag") != std::string::npos) {
      state = State::fragment;
      continue;
    }
    if(state == State::common)
      common += line + "\n";
    else if(state == State::vertex) {
      vertex += line + "\n";;
    }
    else if(state == State::fragment)
      fragment += line + "\n";
  }
  common = parseInludes(common);
  vertex = parseInludes(vertex);
  fragment = parseInludes(fragment);
  const std::string outVertex = common + vertex;
  const std::string outFragment = common + fragment;

  const std::string vf = inFile + ".vert";
  const std::string ff = inFile + ".frag";

  std::ofstream outVertexFile(vf, std::ofstream::out);
  std::ofstream outFragmentFile(ff, std::ofstream::out);
  assert(outVertexFile.is_open() && "could not create .vert file");
  assert(outFragmentFile.is_open() && "could not create .frag file");

  outVertexFile << outVertex;
  outFragmentFile << outFragment;

  outVertexFile.close();
  outFragmentFile.close();

  std::filesystem::create_directory("builds");
  
  const std::string vertexSpv = std::string(std::getenv("VULKAN_SDK")) + "/bin/glslangValidator -V " + vf + " -o builds/" + vf + ".spv";
  const std::string fragmentSpv = std::string(std::getenv("VULKAN_SDK")) + "/bin/glslangValidator -V " + ff + " -o builds/" + ff + ".spv";

  std::cout << vertexSpv << std::endl;

  bool foundError = false;

  std::cout << inFile << std::endl;
  foundError = printErrorAndWarnings(exec(vertexSpv.c_str()));
  if(foundError)
    printShader(outVertex);

  foundError = printErrorAndWarnings(exec(fragmentSpv.c_str()));
  if(foundError)
    printShader(outFragment);

  remove(vf.c_str());
  remove(ff.c_str());

  return 0;
}