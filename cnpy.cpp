// clang-format off

// Copyright (C) 2011  Carl Rogers
// Released under MIT License
// license available in LICENSE file, or at http://www.opensource.org/licenses/mit-license.php

#include "cnpy.h"

#include <zlib.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <istream>
#include <regex>
#include <stdexcept>

namespace cnpy {

static inline void np_assert(bool condition, const std::string& msg)
{
    if (!condition) {
        throw std::runtime_error(msg);
    }
}

static std::ifstream open_stream(std::string fname)
{
    std::ifstream stream(fname, std::ios::binary | std::ios::in);
    if (!stream) {
        throw std::runtime_error("Error opening file '" + fname + "'");
    }
    return stream;
}

static NpyArray load_npz_array(std::istream& stream, uint32_t compr_bytes, uint32_t uncompr_bytes)
{
    std::vector<unsigned char> buffer_compr(compr_bytes);
    std::vector<unsigned char> buffer_uncompr(uncompr_bytes);
    stream.read(reinterpret_cast<char *>(buffer_compr.data()), compr_bytes);
    if (!stream.good()) {
        throw std::runtime_error("load_npz_array: failed to read data");
    }

    z_stream d_stream;

    d_stream.zalloc = Z_NULL;
    d_stream.zfree = Z_NULL;
    d_stream.opaque = Z_NULL;
    d_stream.avail_in = 0;
    d_stream.next_in = Z_NULL;
    inflateInit2(&d_stream, -MAX_WBITS);

    d_stream.avail_in = compr_bytes;
    d_stream.next_in = &buffer_compr[0];
    d_stream.avail_out = uncompr_bytes;
    d_stream.next_out = &buffer_uncompr[0];

    inflate(&d_stream, Z_FINISH);
    inflateEnd(&d_stream);

    std::vector<size_t> shape;
    size_t word_size;
    bool fortran_order;
    cnpy::parse_npy_header(&buffer_uncompr[0], word_size, shape, fortran_order);

    cnpy::NpyArray array(shape, word_size, fortran_order);

    size_t offset = uncompr_bytes - array.num_bytes();
    memcpy(array.data<unsigned char>(), &buffer_uncompr[0] + offset, array.num_bytes());

    return array;
}

}; // namespace cnpy

template <> std::vector<char>& cnpy::operator+=(std::vector<char>& lhs, const std::string& rhs)
{
    lhs.insert(lhs.end(), rhs.begin(), rhs.end());
    return lhs;
}

template <> std::vector<char>& cnpy::operator+=(std::vector<char>& lhs, const char *rhs)
{
    // write in little endian
    size_t len = strlen(rhs);
    lhs.reserve(len);
    for (size_t byte = 0; byte < len; byte++) {
        lhs.push_back(rhs[byte]);
    }
    return lhs;
}

void cnpy::parse_npy_header(
    unsigned char *buffer, size_t& word_size, std::vector<size_t>& shape, bool& fortran_order
)
{
    uint16_t header_len = *reinterpret_cast<uint16_t *>(buffer + 8);
    std::string header(reinterpret_cast<char *>(buffer + 9), header_len);

    size_t loc1, loc2;

    // fortran order
    loc1 = header.find("fortran_order") + 16;
    fortran_order = (header.substr(loc1, 4) == "True" ? true : false);

    // shape
    loc1 = header.find("(");
    loc2 = header.find(")");

    std::regex num_regex("[0-9][0-9]*");
    std::smatch sm;
    shape.clear();

    std::string str_shape = header.substr(loc1 + 1, loc2 - loc1 - 1);
    while (std::regex_search(str_shape, sm, num_regex)) {
        shape.push_back(std::stoi(sm[0].str()));
        str_shape = sm.suffix().str();
    }

    // endian, word size, data type
    // byte order code | stands for not applicable.
    // not sure when this applies except for byte array
    loc1 = header.find("descr") + 9;
    bool littleEndian = (header[loc1] == '<' || header[loc1] == '|' ? true : false);
    np_assert(littleEndian, "file must be little-endian");

    std::string str_ws = header.substr(loc1 + 2);
    loc2 = str_ws.find("'");
    word_size = atoi(str_ws.substr(0, loc2).c_str());
}

void cnpy::parse_npy_header(
    std::istream& stream, size_t& word_size, std::vector<size_t>& shape, bool& fortran_order
)
{
    char buffer[256];
    stream.read(buffer, 11);
    if (!stream.good()) {
        throw std::runtime_error("parse_npy_header: failed to read header");
    }

    std::string header;
    header.resize(255);
    stream.getline(header.data(), 256);

    // getline reads newline and counts it towards gcount() but does not store it
    header.resize(stream.gcount() - 1);
    if (!stream.good()) {
        throw std::runtime_error("parse_npy_header: failed to read header");
    }

    size_t loc1, loc2;

    // fortran order
    loc1 = header.find("fortran_order");
    if (loc1 == std::string::npos)
        throw std::runtime_error("parse_npy_header: failed to find header keyword: 'fortran_order'"
        );
    loc1 += 16;
    fortran_order = (header.substr(loc1, 4) == "True" ? true : false);

    // shape
    loc1 = header.find("(");
    loc2 = header.find(")");
    if (loc1 == std::string::npos || loc2 == std::string::npos)
        throw std::runtime_error("parse_npy_header: failed to find header keyword: '(' or ')'");

    std::regex num_regex("[0-9][0-9]*");
    std::smatch sm;
    shape.clear();

    std::string str_shape = header.substr(loc1 + 1, loc2 - loc1 - 1);
    while (std::regex_search(str_shape, sm, num_regex)) {
        shape.push_back(std::stoi(sm[0].str()));
        str_shape = sm.suffix().str();
    }

    // endian, word size, data type
    // byte order code | stands for not applicable.
    // not sure when this applies except for byte array
    loc1 = header.find("descr");
    if (loc1 == std::string::npos)
        throw std::runtime_error("parse_npy_header: failed to find header keyword: 'descr'");
    loc1 += 9;
    bool littleEndian = (header[loc1] == '<' || header[loc1] == '|' ? true : false);
    np_assert(littleEndian, "file must be little-endian");

    std::string str_ws = header.substr(loc1 + 2);
    loc2 = str_ws.find("'");
    word_size = atoi(str_ws.substr(0, loc2).c_str());
}

cnpy::npz_t cnpy::npz_load(const std::string& fname)
{
    auto stream = cnpy::open_stream(fname);
    return cnpy::npz_load(stream);
}

cnpy::npz_t cnpy::npz_load(std::istream& stream)
{
    cnpy::npz_t arrays;
    while (1) {
        std::vector<char> local_header(30);
        stream.read(local_header.data(), 30);
        if (!stream.good()) {
            throw std::runtime_error("npz_load: failed to read local header");
        }

        // if we've reached the global header, stop reading
        if (local_header[2] != 0x03 || local_header[3] != 0x04)
            break;

        // read in the variable name
        uint16_t name_len = *reinterpret_cast<uint16_t *>(&local_header[26]);
        std::string varname(name_len, ' ');
        stream.read(&varname[0], name_len);
        if (!stream.good()) {
            throw std::runtime_error("npz_load: failed to read variable name");
        }

        // erase the lagging .npy
        varname.erase(varname.end() - 4, varname.end());

        // read in the extra field
        uint16_t extra_field_len = *reinterpret_cast<uint16_t *>(&local_header[28]);
        if (extra_field_len > 0) {
            std::vector<char> buff(extra_field_len);
            stream.read(buff.data(), extra_field_len);
            if (!stream.good()) {
                throw std::runtime_error("npz_load: failed to read extra field");
            }
        }

        uint16_t compr_method = *reinterpret_cast<uint16_t *>(&local_header[0] + 8);
        uint32_t compr_bytes = *reinterpret_cast<uint32_t *>(&local_header[0] + 18);
        uint32_t uncompr_bytes = *reinterpret_cast<uint32_t *>(&local_header[0] + 22);

        if (compr_method == 0) {
            arrays[varname] = npy_load(stream);
        } else {
            arrays[varname] = load_npz_array(stream, compr_bytes, uncompr_bytes);
        }
    }

    return arrays;
}

cnpy::NpyArray cnpy::npz_load(const std::string& fname, const std::string& varname)
{
    auto stream = cnpy::open_stream(fname);
    return cnpy::npz_load(stream, varname);
}

cnpy::NpyArray cnpy::npz_load(std::istream& stream, const std::string& varname)
{
    while (1) {
        std::vector<char> local_header(30);
        stream.read(local_header.data(), 30);
        if (!stream.good()) {
            throw std::runtime_error("npz_load: failed to read local header");
        }

        // if we've reached the global header, stop reading
        if (local_header[2] != 0x03 || local_header[3] != 0x04)
            break;

        // read in the variable name
        uint16_t name_len = *reinterpret_cast<uint16_t *>(&local_header[26]);
        std::string vname(name_len, ' ');
        stream.read(&vname[0], name_len);
        if (!stream.good()) {
            throw std::runtime_error("npz_load: failed to read variable name");
        }
        vname.erase(vname.end() - 4, vname.end()); // erase the lagging .npy

        // read in the extra field
        uint16_t extra_field_len = *reinterpret_cast<uint16_t *>(&local_header[28]);
        stream.seekg(extra_field_len, std::ios_base::cur);
        if (stream.fail()) {
            throw std::runtime_error("npz_load: failed to seek past extra field");
        }

        uint16_t compr_method = *reinterpret_cast<uint16_t *>(&local_header[0] + 8);
        uint32_t compr_bytes = *reinterpret_cast<uint32_t *>(&local_header[0] + 18);
        uint32_t uncompr_bytes = *reinterpret_cast<uint32_t *>(&local_header[0] + 22);

        if (vname == varname) {
            NpyArray array = (compr_method == 0) ?
                npy_load(stream) :
                load_npz_array(stream, compr_bytes, uncompr_bytes);
            return array;
        } else {
            // skip past the data
            uint32_t size = *reinterpret_cast<uint32_t *>(&local_header[22]);
            stream.seekg(size, std::ios_base::cur);
            if (stream.fail()) {
                throw std::runtime_error("npz_load: failed to seek past data");
            }
        }
    }

    // if we get here, we haven't found the variable in the file
    throw std::runtime_error("npz_load: Variable name " + varname + " not found");
}

cnpy::NpyArray cnpy::npy_load(const std::string& fname)
{
    auto stream = cnpy::open_stream(fname);
    return cnpy::npy_load(stream);
}

cnpy::NpyArray cnpy::npy_load(std::istream& stream)
{
    std::vector<size_t> shape;
    size_t word_size;
    bool fortran_order;
    cnpy::parse_npy_header(stream, word_size, shape, fortran_order);

    cnpy::NpyArray arr(shape, word_size, fortran_order);
    stream.read(arr.data<char>(), arr.num_bytes());
    if (!stream.good()) {
        throw std::runtime_error("npy_load: failed to read data");
    }
    return arr;
}
