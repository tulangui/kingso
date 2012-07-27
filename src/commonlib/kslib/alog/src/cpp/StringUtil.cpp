#include <iterator>
#include <stdio.h>
#include <stdlib.h>
#include "StringUtil.h"

namespace alog {

std::string StringUtil::trim(const std::string& s) {
    static const char* whiteSpace = " \t\r\n";

    // test for null string
    if (s.empty())
        return s;

    // find first non-space character
    std::string::size_type b = s.find_first_not_of(whiteSpace);
    if (b == std::string::npos) // No non-spaces
        return "";

    // find last non-space character
    std::string::size_type e = s.find_last_not_of(whiteSpace);

    // return the remaining characters
    return std::string(s, b, e - b + 1);
}

unsigned int StringUtil::split(std::vector<std::string>& v,
                               const std::string& s,
                               char delimiter, unsigned int maxSegments) {
    v.clear();
    std::back_insert_iterator<std::vector<std::string> > it(v);
    return split(it, s, delimiter, maxSegments);
}

}
