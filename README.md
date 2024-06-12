(This repository is a fork of [Carl Rogers's `cnpy` library](https://github.com/rogersce/cnpy/).
It adds modern CMake support, removes the writing functionality, and supports
`std::istream`s.)

# Purpose:

NumPy offers the `save` method for easy saving of arrays into .npy and `savez`
for zipping multiple .npy arrays together into a .npz file.

`cnpy` lets you read and write to these formats in C++.

The motivation comes from scientific programming where large amounts of data are
generated in C++ and analyzed in Python.

Writing to .npy has the advantage of using low-level C++ I/O (fread and fwrite)
for speed and binary format for size. The .npy file header takes care of
specifying the size, shape, and data type of the array, so specifying the format
of the data is unnecessary.

Loading data written in numpy formats into C++ is equally simple, but requires
you to type-cast the loaded data to the type of your choice.

# Description:

There are 3 functions for reading:

- `npy_load(file)` will load a .npy file.
- `npz_load(file)` will load a .npz and return a dictionary of NpyArray
  structues.
- `npz_load(file, varname)` will load and return the NpyArray for data varname
  from the specified .npz file.

The `file` argument can be a string of the path to load or an `std::istream&`.

The data structure for loaded data is below. Data is accessed via the
`data<T>()`-method, which returns a pointer of the specified type (which must
match the underlying datatype of the data). The array shape and word size are
read from the npy header.

```c++
struct NpyArray {
    std::vector<size_t> shape;
    size_t word_size;
    template<typename T> T* data();
};
```

Writing functionality has been (temporarily) removed from this fork, see the
original (https://github.com/rogersce/cnpy/) for saving functions. Note that
these functions must take a path string argument and don't support
`std::ostream`.

# Usage

Requires [zlib](https://zlib.net/).

You can build and install this library on your system, or you can include it
directly into your CMake project using FetchContent (recommended).

## Installing static library

```sh
mkdir build
cmake path/to/this/dir  # add -DCMAKE_INSTALL_PREFIX=<install dir> to change installation directory
make
make install
```

Then compile with `-lcnpy` and `-lz`, or from CMake:

```cmake
find_package(cnpy REQUIRED)
target_link_libraries(... cnpy::cnpy)
```

## FetchContent

To include this library directly in your CMake project, use FetchContent:

```cmake
include(FetchContent)
fetchcontent_declare(
    cnpy
    GIT_REPOSITORY https://github.com/harrymander/cnpy
    GIT_TAG <tag or SHA>
)
fetchcontent_makeavailable(cnpy)
target_link_libraries(... cnpy::cnpy)
```
