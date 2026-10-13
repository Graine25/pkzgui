# pkzgui
```sh
git submodule update --init --recursive
```

Build and run the ImGui app:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DGLFW_BUILD_WAYLAND=OFF
cmake --build build --parallel
./build/bin/pkzgui
```
