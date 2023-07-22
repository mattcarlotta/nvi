# Contributing

1. [Fork it](https://help.github.com/articles/fork-a-repo/)
2. Install [requirements](README.md#requirements)
3. Create your feature branch (`git checkout -b my-new-feature`)
4. Commit your changes (`git commit -am 'Added some feature'`)
5. Test your changes 
6. Push to the branch (`git push origin my-new-feature`)
7. [Create new Pull Request](https://help.github.com/articles/creating-a-pull-request/)

## Testing

This project uses [googletest](https://github.com/google/googletest) to write run unit tests. Run the test suite with these commands:

```DOSINI
# at the project root directory
cmake -DCOMPILE_TESTS=ON . 
make 
cd tests 
./tests
```

## Code Style

This project uses [clang-format](https://clang.llvm.org/docs/ClangFormatStyleOptions.html) to maintain code style and best practices and is loosely based upon the recommendations in openstack's [Cpp Standards](https://wiki.openstack.org/wiki/CppCodingStandards).
