#ifndef NVI_FORMAT_H
#define NVI_FORMAT_H

#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace nvi::fmt {
    /**
     * @detail Joins a list of elements into a single string.
     *
     * @param `elements` a list of elements to be combined.
     *
     * @param `delimiter` a character to use to separate each item in the string.
     *
     * @return a string.
     */
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
}; // namespace nvi::fmt

#endif
