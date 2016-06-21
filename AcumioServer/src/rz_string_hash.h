#ifndef AcumioServer_rz_string_hash_h
#define AcumioServer_rz_string_hash_h
//============================================================================
// Name        : transaction.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : This file contains the Functional RZStringHash - which form
//               the basis of Ramakrishna-Zobel class of hashing functions.
//               See: "Performance in Practice of String Hashing Functions"
//
//               The intent here is simply for making hash-tables, and is
//               not intended for security purposes. Note that it generates
//               hash values in the range [0, 2^32), which is too small a
//               space for security purposes.
//
//               An observation is that the hash function requires the
//               entire string length to complete, which may not be
//               particularly efficient. Adaptive-length techniques may
//               perform better.
//
//               According to the original paper on this topic, we need to
//               compute the mod of the final result over the size of the
//               hash-table. This is fine, but it's better to leave the
//               hash function itself unaware of the table size.
//============================================================================

#include <string>

namespace acumio {
namespace util {
class RZStringHash {
 public:
  // seed can be any value from uint32_t, left_shift should be in the
  // range 4 <= left_shift <= 7, and right_shift should be in the range
  // 1 <= right_shift <= 3.
  // This class will not enforce these constraints, but violating the
  // constraints will possibly give uneven results.
  RZStringHash(uint32_t seed, uint8_t left_shift, uint8_t right_shift) :
      seed_(seed), left_shift_(left_shift), right_shift_(right_shift) {}
  ~RZStringHash();

  uint32_t operator()(const std::string& s);

  inline uint32_t seed() { return seed_; }
  inline uint8_t left_shift() { return left_shift_; }
  inline uint8_t right_shift() { return right_shift_; }
  // next_hash_value =
  //    current_hash xor
  //    (left-shift(current_hash) + right-shift(current_hash) + c)
  inline uint32_t next_hash_value(uint32_t h, char c) {
    return h ^ ((h << left_shift_) + (h >> right_shift_) + c);
  }

 private:
  uint32_t seed_;
  uint8_t left_shift_;
  uint8_t right_shift_;
};

} // namespace util
} // namespace acumio
#endif // AcumioServer_rz_string_hash_h
