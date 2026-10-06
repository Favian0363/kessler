Builds: 
Normal (Slow, Sanitizers)
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure

Release (Faster)
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j --target kessler_screen
