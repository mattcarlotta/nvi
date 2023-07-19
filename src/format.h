#ifndef NVI_FORMAT_H
#define NVI_FORMAT_H

#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace nvi {
template <typename... A> std::string format(const char *fmt, A... args) {
    size_t size = snprintf(nullptr, 0, fmt, args...);
    std::string buf;
    buf.reserve(size + 1);
    buf.resize(size);
    snprintf(&buf[0], size + 1, fmt, args...);
    return buf;
}

template <typename E> std::string join(E const &elements, const char *const delimiter) {
    std::ostringstream os;
    auto b = std::begin(elements);
    auto e = std::end(elements);

    if (b != e) {
        std::copy(b, std::prev(e), std::ostream_iterator<std::string>(os, delimiter));
        b = std::prev(e);
    }

    if (b != e) {
        os << *b;
    }

    return os.str();
}
}; // namespace nvi

#endif
