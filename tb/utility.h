//========================================================================== //
// Copyright (c) 2020, Stephen Henry
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//========================================================================== //

#ifndef M_TB_UTILITY_H
#define M_TB_UTILITY_H

#include <string>
#include <vector>
#include <utility>

namespace tb::utility {

template<typename T>
T mask(std::size_t n) {
  if (sizeof(T) * 8 == n) return ~0;

  return (static_cast<T>(1) << n) - 1;
}

//
class KVListRenderer {
  using kv_type = std::pair<std::string, std::string>;

 public:
  KVListRenderer() = default;

  //
  std::string to_string() const;

  //
  void add_field(const std::string& key, const std::string& value);

 private:
  // Key/Value pairs
  std::vector<kv_type> kvs_;
};


//
//
struct Hexer {
  explicit Hexer() = default;

  // Currently rendering uppercase.
  bool upper_case() const { return upper_case_; }

  // Currently rendering with prefix.
  bool prefix() const { return prefix_; }

  // Convert some multi-byte value to hex equivalent.
  std::string to_hex(std::uint64_t x, std::size_t bits = 64) const;
  std::string to_hex(const char* c, std::size_t bits) const;

  // Convert integer in range [0, 16) to hex character.
  char hex_char(char x) const;

 private:
  // Render test with uppercase hexadecimal characters.
  bool upper_case_ = false;

  // Render text with '0x' prefix
  bool prefix_ = true;

  // Render with stripping leading zeros
  bool truncate_ = true;
};

const char* to_string(bool b);

} // namespace tb::utility

#endif
