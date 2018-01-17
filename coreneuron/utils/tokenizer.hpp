#pragma once

#include <string>
#include <vector>

std::vector<std::string> tokenize ( const char* str,
    const char separator,
    const char skip = ' ') {
  std::vector<std::string> results;
  do
  {
    //skip blanks and separator before the string
    while ((*str == skip || *str == separator) && *str)
      str++;
    const char *begin = str;
    //stop at blank or separator
    while(*str != separator && *str && *str != skip)
      str++;
    if (begin != str)
      results.push_back(std::string(begin, str));

  } while (0 != *str++);

  return results;
}
