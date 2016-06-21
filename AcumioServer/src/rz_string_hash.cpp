//============================================================================
// Name        : rz_string_hash.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Implementation of RZStringHash class.
//============================================================================

#include "rz_string_hash.h"

namespace acumio {
namespace util {

RZStringHash::~RZStringHash() {}

uint32_t RZStringHash::operator()(const std::string& s) {
  uint32_t h = seed_;
  for (auto it = s.begin(); it != s.end(); it++) {
    h = next_hash_value(h, *it);
  }
  return h;
}

} // namespace util
} // namespace acumio
