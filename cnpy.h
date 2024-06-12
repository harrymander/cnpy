// Copyright (C) 2011  Carl Rogers
// Released under MIT License
// license available in LICENSE file, or at http://www.opensource.org/licenses/mit-license.php

#ifndef LIBCNPY_H_
#define LIBCNPY_H_

#include <cassert>
#include <cstdint>
#include <iostream>
#include <istream>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

namespace cnpy {

struct NpyArray {
    NpyArray(const std::vector<size_t>& _shape, size_t _word_size, bool _fortran_order) :
        shape(_shape), word_size(_word_size), fortran_order(_fortran_order)
    {
        num_vals = 1;
        for (size_t i = 0; i < shape.size(); i++)
            num_vals *= shape[i];
        data_holder =
            std::shared_ptr<std::vector<char>>(new std::vector<char>(num_vals * word_size));
    }

    NpyArray() : shape(0), word_size(0), fortran_order(0), num_vals(0) {}

    template <typename T> T *data() { return reinterpret_cast<T *>(&(*data_holder)[0]); }

    template <typename T> const T *data() const
    {
        return reinterpret_cast<T *>(&(*data_holder)[0]);
    }

    template <typename T> std::vector<T> as_vec() const
    {
        const T *p = data<T>();
        return std::vector<T>(p, p + num_vals);
    }

    size_t num_bytes() const { return data_holder->size(); }

    std::shared_ptr<std::vector<char>> data_holder;
    std::vector<size_t> shape;
    size_t word_size;
    bool fortran_order;
    size_t num_vals;
};

using npz_t = std::map<std::string, NpyArray>;

char BigEndianTest();
char map_type(const std::type_info& t);
template <typename T> std::vector<char> create_npy_header(const std::vector<size_t>& shape);
void parse_npy_header(
    std::istream& stream, size_t& word_size, std::vector<size_t>& shape, bool& fortran_order
);
void parse_npy_header(
    unsigned char *buffer, size_t& word_size, std::vector<size_t>& shape, bool& fortran_order
);
void parse_zip_footer(
    std::istream& stream, uint16_t& nrecs, size_t& global_header_size, size_t& global_header_offset
);

npz_t npz_load(const std::string& fname);
npz_t npz_load(std::istream& stream);

NpyArray npz_load(const std::string& fname, const std::string& varname);
NpyArray npz_load(std::istream& stream, const std::string& varname);

NpyArray npy_load(const std::string& fname);
NpyArray npy_load(std::istream& stream);

template <typename T> std::vector<char>& operator+=(std::vector<char>& lhs, const T rhs)
{
    // write in little endian
    for (size_t byte = 0; byte < sizeof(T); byte++) {
        char val = *(reinterpret_cast<char *>(&rhs) + byte);
        lhs.push_back(val);
    }
    return lhs;
}

template <> std::vector<char>& operator+=(std::vector<char>& lhs, const std::string& rhs);
template <> std::vector<char>& operator+=(std::vector<char>& lhs, const char *rhs);
} // namespace cnpy

#endif
