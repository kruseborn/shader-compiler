#pragma once
#include <string>

#define mgAssertDesc(cond, msg)                                                \
  do {                                                                         \
    if (!(cond)) {                                                             \
      printf(msg);                                                             \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

void removeSubstring(std::string &str, const std::string &substr);
void printShader(const std::string &shader);
uint32_t nrOfLines(std::string in);
bool findString(const std::string &inStr, const std::string &pattern);
std::string exec(const char *cmd);
std::string getErrorAndWarnings(const std::string strOutput);
bool exists(const std::string &name);
std::string parseInludes(std::string &in);