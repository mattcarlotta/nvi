#ifndef NVI_FORMAT_H
#define NVI_FORMAT_H
#include <string>

namespace nvi {
template <typename... Args> std::string format(const char *fmt, Args... args) {
    size_t size = snprintf(nullptr, 0, fmt, args...);
    std::string buf;
    buf.reserve(size + 1);
    buf.resize(size);
    snprintf(&buf[0], size + 1, fmt, args...);
    return buf;
}
}; // namespace nvi

#endif
