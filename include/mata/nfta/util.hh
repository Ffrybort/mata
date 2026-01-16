/**
 * @brief Helper functions
 */

#ifndef NFTA_UTIL_HH
#define NFTA_UTIL_HH

#include <string>
#include <cctype>

/**
 * @brief Removes all whitespace characters from a string.
 */
inline void removeWhitespace(std::string& s)
{
    std::erase_if(s, [](unsigned char c){ return std::isspace(c); });
}

/**
 * @brief Trims leading and trailing whitespace from a string.
 */
inline void trimWhitespace(std::string& s)
{
    const auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        s.clear(); // string is all whitespace
        return;
    }

    const auto end = s.find_last_not_of(" \t\n\r");
    s = s.substr(start, end - start + 1);
}

#endif // NFTA_UTIL_HH
