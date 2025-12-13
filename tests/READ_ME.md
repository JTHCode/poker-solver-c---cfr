**To run all unit tests:**

`cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug`       
`cmake --build build`                                                     
`ctest --test-dir build --output-on-failure`