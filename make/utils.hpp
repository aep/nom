#include <string>
#include <iterator>
#include <vector>
#include <algorithm>

inline bool endsWith (std::string const &fullString, std::string const &ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

inline bool startsWith (std::string const &fullString, std::string const &start)
{
    return (fullString.substr(0, start.size()) == start);
}

template <typename T>
inline void amend(std::vector<T> &dst, const std::vector<T> &src)
{
    std::vector<T> tmp;
    tmp.reserve(src.size() + dst.size());
    std::merge(dst.begin(), dst.end(), src.begin(), src.end(), std::back_inserter(tmp));
    dst.swap(tmp);
}
