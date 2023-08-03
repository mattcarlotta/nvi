#!/bin/bash

rm -rf bin cmake_install.cmake CMakeCache.txt CMakeFiles/ CTestTestfile.cmake install_manifest.txt lib Makefile
cd src
rm -rf cmake_install.cmake CMakeCache.txt CMakeFiles/ Makefile nvi
cd ../tests
rm -rf cmake_install.cmake CMakeFiles/ CTestTestfile.cmake libnvi_test.a Makefile tests


