/** \file

    write a zip file in one pass (compression could be easily enabled via zlib)
*/

#ifndef ZIP_BUILDER_GRAEHL_HPP
#define ZIP_BUILDER_GRAEHL_HPP
#pragma once

#include <boost/crc.hpp>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace graehl {

static_assert('ABCD' == 0x41424344, "little endian only");

/** \post little-endian bytes of x are appended to buf.
    presumably assembling first and writing to file once is faster than many small writes. */
template <typename T>
void append_bytes(std::string& buf, T const& x) {
  buf.append((char*)&x, sizeof(T));
}

struct bytes32 {
  char const* bytes;
  std::uint32_t len;
#ifdef _MSC_VER
  bytes32() = default;
  bytes32(char const* bytes, std::uint32_t len)
      : bytes(bytes)
      , len(len) {}
#endif
};

/// manage CRC32 and zip headers for a zipped file
struct zipped_file {
  FILE* zipfile;
  std::string header;
  boost::crc_32_type crc;
  std::uint32_t nbytes = 0;
  std::uint32_t header_pos;
  std::uint16_t fname_bytes;
  static constexpr std::uint32_t PKZIP_HEADER = 0x04034b50u;
  static constexpr std::uint32_t PKZIP_CENTRAL_DIR = 0x02014b50u;
  static constexpr std::uint16_t PKZIP_VERSION = 20;
  static constexpr unsigned HEADER_CRC_OFFSET = 14;
  static constexpr unsigned HEADER_NAME_OFFSET = 14;
  zipped_file(FILE* zipfile, std::uint32_t zipfile_pos)
      : zipfile(zipfile)
      , header_pos(zipfile_pos) {
    assert(zipfile_pos == std::ftell(zipfile));
    header.reserve(90);
    append_bytes(header, PKZIP_HEADER);
    append_bytes(header, PKZIP_VERSION);
    /* all 0 2 bytes ea:
     * general purpose bit flag
     * compression method (uncompressed)
     * file last mod time
     * file last mod date
     */
    header.resize(HEADER_CRC_OFFSET, (char)0);
  }
  std::vector<bytes32> sections;
  void section(char const* data, std::uint32_t len) {
    crc.process_bytes(data, len);
    nbytes += len;
    sections.emplace_back(data, len);
  }
  // \return ftell(zipfile) after writing local header, contents
  std::uint32_t write(std::string const& fname) {
    append_bytes(header, (std::uint32_t)crc()); // crc
    append_bytes(header, nbytes); // compressed size
    append_bytes(header, nbytes); // uncompressed size
    fname_bytes = (std::uint16_t)fname.size();
    assert(fname_bytes == fname.size());
    append_bytes(header, fname_bytes); // fname length
    append_bytes(header, (std::uint16_t)0); // extra field length
    header.append(fname);
    auto zipfile_pos = header_pos;
    write(header.data(), header.size(), zipfile_pos);
    for (auto const& section : sections)
      write(section.bytes, section.len, zipfile_pos);
    assert(zipfile_pos == std::ftell(zipfile));
    return zipfile_pos;
  }
  /// \return #bytes written
  std::uint32_t write_central_directory_entry() const {
    std::string b;
    b.reserve(header.size() + 16);
    append_bytes(b, PKZIP_CENTRAL_DIR);
    append_bytes(b, (std::uint16_t)0);
    auto local = header.data();
    auto local_fname = local + 30;
    b.append(local + 4, local_fname);
    b.resize(6 + (30 - 4) + 10);
    append_bytes(b, header_pos);
    b.append(local_fname, local_fname + fname_bytes);
    std::uint32_t bytes = b.size();
    write(b.data(), bytes);
    return bytes;
  }

 private:
  void write(char const* data, std::uint32_t len) const { std::fwrite(data, sizeof(char), len, zipfile); }
  void write(char const* data, std::uint32_t len, std::uint32_t& pos) const {
    std::fwrite(data, sizeof(char), len, zipfile);
    pos += len;
    assert(pos == std::ftell(zipfile));
  }
};

struct zip_builder {
  FILE* zipfile = 0;
  std::uint32_t zipfile_pos;
  std::vector<zipped_file> files;
  std::string opened_name_storage;
  zipped_file* opened = 0;
  zip_builder() = default;
  /** \pre finish() was called or no zipfile_path provided yet
   */
  void start(std::string const& zipfile_path) { start(zipfile_path.c_str()); }
  void start(char const* zipfile_path) {
    assert(!zipfile);
    zipfile = std::fopen(zipfile_path, "wb");
    assert(ftell(zipfile) == 0);
    zipfile_pos = 0;
    if (!zipfile_path)
      throw std::runtime_error(std::string("couldn't open zip file for output: ") + zipfile_path);
  }
  zip_builder(char const* zipfile_path) { start(zipfile_path); }
  zip_builder(std::string const& zipfile_path)
      : zip_builder(zipfile_path.c_str()) {}
  /// start writing sections to a new unnamed file in the zip
  void open() {
    assert(zipfile);
    if (files.size() >= 65535)
      throw std::runtime_error("zip file with more than 65535 files not supported");
    opened = &files.emplace_back(zipfile, zipfile_pos);
  }
  void section(char const* data, std::uint32_t len) {
    assert(opened);
    opened->section(data, len);
  }
  /** close, save, and name the file in the zip containing all the section(...) content since open.
      no protection against duplicate filenames (zip files allow them) */
  void close(std::string const& filename) {
    assert(opened);
    zipfile_pos = opened->write(filename);
    opened = 0;
  }
  static constexpr std::uint32_t PKZIP_FOOTER = 0x06054b50u;
  static constexpr unsigned PKZIP_FOOTER_BYTES = 22;
  /** write the zip file central directory and footer
      (error if open is called again without a start(zipfile_path) first) */
  void finish() {
    auto central_directory_pos = zipfile_pos;
    std::uint32_t central_directory_len = 0;
    for (auto const& file : files)
      central_directory_len += file.write_central_directory_entry();
    // zip footer
    std::string f;
    append_bytes(f, PKZIP_FOOTER);
    append_bytes(f, (std::uint32_t)0);
    std::uint16_t nrecs = (std::uint16_t)files.size();
    assert(nrecs == files.size());
    append_bytes(f, nrecs);
    append_bytes(f, nrecs);
    append_bytes(f, central_directory_len);
    append_bytes(f, central_directory_pos);
    append_bytes(f, (std::uint16_t)0); // zip comment
    assert(f.size() == PKZIP_FOOTER_BYTES);
    std::fwrite(f.data(), sizeof(char), PKZIP_FOOTER_BYTES, zipfile);
    std::fclose(zipfile);
    files.clear();
    zipfile = 0;
  }
  void file(std::string const& filename, char const* part1_bytes, std::uint32_t part1_len) {
    open();
    section(part1_bytes, part1_len);
    close(filename);
  }
  void file(std::string const& filename, char const* part1_bytes, std::uint32_t part1_len,
            char const* part2_bytes, std::uint32_t part2_len) {
    open();
    section(part1_bytes, part1_len);
    section(part2_bytes, part2_len);
    close(filename);
  }
  ~zip_builder() {
    if (zipfile)
      finish();
  }
};

} // namespace graehl

#endif
