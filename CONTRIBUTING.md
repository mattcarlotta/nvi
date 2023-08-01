# Contributing

1. [Fork it](https://help.github.com/articles/fork-a-repo/)
2. Install [requirements](README.md#requirements)
3. Create your feature branch (`git checkout -b feat/my-new-feature`)
4. Commit your changes (`git commit -am 'feat: Added some awesome new feature'`)
5. Add tests and test your changes
    1. Ensure you've installed the `googletest` dependencies by either cloning the source with `--recursive` or run `git submodule update --init --recursive`
    2. Set the custom cmake flag for compiling tests to ON: `cmake -DCOMPILE_TESTS=ON .` 
    3. Compile the tests binary: `make`
    4. Change directory into tests: `cd tests`
    5. Run the tests: `./tests`
6. If the tests pass, push to the feature branch (`git push origin feat/my-new-feature`)
7. [Create A Pull Request](https://help.github.com/articles/creating-a-pull-request/)

## Commit Style

This project adheres to the [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/#summary) guidelines.

## Testing

This project uses [googletest](https://github.com/google/googletest) to run unit tests.

## Code Style

This project uses [clang-format](https://clang.llvm.org/docs/ClangFormatStyleOptions.html) to maintain code style and best practices and is loosely based upon the recommendations in openstack's [Cpp Standards](https://wiki.openstack.org/wiki/CppCodingStandards).
