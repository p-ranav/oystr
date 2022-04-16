# oystr

`oystr` is a command-line tool that recursively searches directories for a substring.

* Fast case-sensitive search for string literal
* Recursive - ignores certain directories (e.g., `.git`, `build`) and file extensions (e.g., `.pdf`)
* Uses `AVX512F` or `AVX2` SIMD instructions if possible

<p align="center">
  <img src="images/demo.png"/>  
</p>

## Quick Start

Build `oystr` using CMake. For more details, see [BUILDING.md](https://github.com/p-ranav/oystr/blob/master/BUILDING.md).

```bash
git clone https://github.com/p-ranav/oystr
cd oystr

# Build
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build

# Install
sudo cmake --install build
```
