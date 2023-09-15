#pragma once

#include <iostream>
#include <string>

const std::string WHITESPACE = " \n\r\t\f\v";

std::string capitalize(const std::string& str);
std::string ltrim(const std::string& s);
std::string rtrim(const std::string& s);
std::string trim(const std::string& s);
