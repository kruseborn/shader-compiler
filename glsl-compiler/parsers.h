#pragma once
#include <fstream>
#include <string>

void parseRaytracer(std::string fileName, std::ifstream fstream);
void parseCompute(const std::string &fileName, std::ifstream fstream);
void parseRasterization(std::string fileName, std::ifstream fstream);