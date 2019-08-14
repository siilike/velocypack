////////////////////////////////////////////////////////////////////////////////
/// @brief Library to build up VPack documents.
///
/// DISCLAIMER
///
/// Copyright 2015 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Max Neunhoeffer
/// @author Jan Steemann
/// @author Copyright 2015, ArangoDB GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef VELOCYPACK_STRINGREF_H
#define VELOCYPACK_STRINGREF_H 1

#include <cstring>
#include <functional>
#include <algorithm>
#include <string>
#include <iosfwd>
#include <iterator>

#include "velocypack/velocypack-common.h"

namespace arangodb {
namespace velocypack {
class Slice;

class StringRef {
 public:
  class const_iterator {
   public:
    typedef const_iterator self_type;
    typedef char value_type;
    typedef char const& reference;
    typedef char const* pointer;
    typedef std::random_access_iterator_tag iterator_category;
    typedef int difference_type;
    const_iterator(pointer ptr) : _ptr(ptr) {}
    const_iterator(const_iterator const& rhs) : _ptr(rhs._ptr) {}
    const_iterator& operator=(const_iterator const& rhs) { _ptr = rhs._ptr; return *this; }
    self_type operator++() { _ptr++; return *this; }
    self_type operator--() { _ptr--; return *this; }
    self_type operator++(int) { self_type i = *this; _ptr++; return i; }
    self_type operator--(int) { self_type i = *this; _ptr--; return i; }
    self_type operator+=(int value) { _ptr += value; return *this; }
    self_type operator-=(int value) { _ptr -= value; return *this; }
    reference operator*() { return *_ptr; }
    pointer operator->() { return _ptr; }
    reference operator[](std::size_t index) { return _ptr[index]; }
    bool operator==(self_type const& rhs) { return _ptr == rhs._ptr; }
    bool operator!=(self_type const& rhs) { return _ptr != rhs._ptr; }
    bool operator>(self_type const& rhs) { return _ptr > rhs._ptr; }
    bool operator>=(self_type const& rhs) { return _ptr >= rhs._ptr; }
    bool operator<(self_type const& rhs) { return _ptr < rhs._ptr; }
    bool operator<=(self_type const& rhs) { return _ptr <= rhs._ptr; }
   private:
    pointer _ptr;
  };

  /// @brief create an empty StringRef
  constexpr StringRef() noexcept : _data(""), _length(0) {}

  /// @brief create a StringRef from an std::string
  explicit StringRef(std::string const& str) noexcept : StringRef(str.data(), str.size()) {}
  
  /// @brief create a StringRef from a C string plus length
  constexpr StringRef(char const* data, std::size_t length) noexcept : _data(data), _length(length) {}
  
  /// @brief create a StringRef from a null-terminated C string
#if __cplusplus >= 201703
  constexpr explicit StringRef(char const* data) noexcept : StringRef(data, std::char_traits<char>::length(data)) {}
#else
  explicit StringRef(char const* data) noexcept : StringRef(data, strlen(data)) {}
#endif
   
  /// @brief create a StringRef from a VPack slice (must be of type String)
  explicit StringRef(Slice slice);
  
  /// @brief create a StringRef from another StringRef
  constexpr StringRef(StringRef const& other) noexcept
      : _data(other._data), _length(other._length) {}
  
  /// @brief move a StringRef from another StringRef
  constexpr StringRef(StringRef&& other) noexcept
      : _data(other._data), _length(other._length) {}
  
  /// @brief create a StringRef from another StringRef
  StringRef& operator=(StringRef const& other) noexcept {
    _data = other._data;
    _length = other._length;
    return *this;
  }
  
  /// @brief move a StringRef from another StringRef
  StringRef& operator=(StringRef&& other) noexcept {
    _data = other._data;
    _length = other._length;
    return *this;
  }
  
  /// @brief create a StringRef from an std::string
  StringRef& operator=(std::string const& other) noexcept {
    _data = other.data();
    _length = other.size();
    return *this;
  }
  
  /// @brief create a StringRef from a null-terminated C string
  StringRef& operator=(char const* other) noexcept {
    _data = other;
    _length = strlen(other);
    return *this;
  }
  
  /// @brief create a StringRef from a VPack slice of type String
  StringRef& operator=(Slice slice);
  
  StringRef substr(std::size_t pos = 0, std::size_t count = std::string::npos) const;
  
  char at(std::size_t index) const;
  
  std::size_t find(char c) const;
  
  std::size_t rfind(char c) const;

  int compare(StringRef const& other) const noexcept;
  
  int compare(std::string const& other) const noexcept { return compare(StringRef(other)); }
  
  int compare(char const* other) const noexcept { return compare(StringRef(other)); }

  bool equals(StringRef const& other) const noexcept;
  
  bool equals(std::string const& other) const noexcept { return equals(StringRef(other)); }
  
  bool equals(char const* other) const noexcept { return equals(StringRef(other)); }

  inline std::string toString() const {
    return std::string(_data, _length);
  }

  constexpr inline bool empty() const noexcept {
    return (_length == 0);
  }
 
  inline const_iterator begin() const noexcept {
    return const_iterator(_data);
  }
  
  inline const_iterator cbegin() const noexcept {
    return const_iterator(_data);
  }
 
  inline const_iterator end() const noexcept {
    return const_iterator(_data + _length);
  }
  
  inline const_iterator cend() const noexcept {
    return const_iterator(_data + _length);
  }

  inline char front() const noexcept { return _data[0]; }

  inline char back() const noexcept { return _data[_length - 1]; }

  inline void pop_back() { --_length; }
  
  inline char operator[](std::size_t index) const noexcept { 
    return _data[index];
  }
  
  constexpr inline char const* data() const noexcept {
    return _data;
  }

  constexpr inline std::size_t size() const noexcept {
    return _length;
  }

  constexpr inline std::size_t length() const noexcept {
    return _length;
  }

 private:
  char const* _data;
  std::size_t _length;
};

std::ostream& operator<<(std::ostream& stream, StringRef const& ref);
} // namespace velocypack
} // namespace arangodb

inline bool operator==(arangodb::velocypack::StringRef const& lhs, arangodb::velocypack::StringRef const& rhs) {
  return (lhs.size() == rhs.size() && memcmp(lhs.data(), rhs.data(), lhs.size()) == 0);
}

inline bool operator!=(arangodb::velocypack::StringRef const& lhs, arangodb::velocypack::StringRef const& rhs) {
  return !(lhs == rhs);
}

inline bool operator==(arangodb::velocypack::StringRef const& lhs, std::string const& rhs) {
  return (lhs.size() == rhs.size() && memcmp(lhs.data(), rhs.c_str(), lhs.size()) == 0);
}

inline bool operator!=(arangodb::velocypack::StringRef const& lhs, std::string const& rhs) {
  return !(lhs == rhs);
}

inline bool operator==(arangodb::velocypack::StringRef const& lhs, char const* rhs) {
  std::size_t const len = strlen(rhs);
  return (lhs.size() == len && memcmp(lhs.data(), rhs, lhs.size()) == 0);
}

inline bool operator!=(arangodb::velocypack::StringRef const& lhs, char const* rhs) {
  return !(lhs == rhs);
}

inline bool operator<(arangodb::velocypack::StringRef const& lhs, arangodb::velocypack::StringRef const& rhs) {
  return (lhs.compare(rhs) < 0);
}

inline bool operator>(arangodb::velocypack::StringRef const& lhs, arangodb::velocypack::StringRef const& rhs) {
  return (lhs.compare(rhs) > 0);
}

namespace std {

template <>
struct hash<arangodb::velocypack::StringRef> {
  std::size_t operator()(arangodb::velocypack::StringRef const& value) const noexcept {
    return VELOCYPACK_HASH(value.data(), value.size(), 0xdeadbeef); 
  }
};

template <>
struct equal_to<arangodb::velocypack::StringRef> {
  bool operator()(arangodb::velocypack::StringRef const& lhs,
                  arangodb::velocypack::StringRef const& rhs) const noexcept {
    return (lhs.size() == rhs.size() &&
            (memcmp(lhs.data(), rhs.data(), lhs.size()) == 0));
  }
};

}

#endif
