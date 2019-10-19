#include "utils.h"
#include <algorithm>
#include <fstream>
#include <memory>
#include <sstream>

void removeSubstring(std::string &str, const std::string &substr) {
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
  auto it = std::search(inStr.begin(), inStr.end(), pattern.begin(), pattern.end(),
                        [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); });
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
