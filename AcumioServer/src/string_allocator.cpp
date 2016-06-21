//============================================================================
// Name        : string_allocator.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : StringAllocator implementation.
//============================================================================

#include "string_allocator.h"
#include <cstdint>
#include <cstring>

namespace acumio {
namespace collection {

StringAllocator::StringAllocator(uint16_t max_size) : max_size_(max_size),
    space_used_(0), key_list_(nullptr), reference_counts_(nullptr),
    free_key_list_() {
  if (max_size_ > 0) {
    key_list_ = new char[max_size_];
    reference_counts_ = new uint8_t[max_size_];
  }
  FreeKey initial_free_key(0, max_size_);
  free_key_list_.push_back(initial_free_key);
}

StringAllocator::~StringAllocator() {
  if (max_size_ > 0) {
    delete [] key_list_;
    delete [] reference_counts_;
  }
}

uint16_t StringAllocator::Add(const char* s) {
  uint16_t space_needed = strlen(s) + 1;
  int16_t last_free_pos = free_key_list_.size() - 1;
  int16_t free_pos = last_free_pos;
  uint16_t hole_length = 0;
  while (free_pos >= 0 &&
         (hole_length = free_key_list_[free_pos].hole_length) < space_needed) {
    free_pos--;
  }
  if (free_pos < 0) {
    return max_size_;
  }
  FreeKey& updated_free = free_key_list_[free_pos];
  uint16_t ret_val = updated_free.location;
  uint16_t offset = ret_val;
  for (const char* i = s; *i != '\0'; i++) {
    key_list_[offset++] = *i;
  }
  key_list_[offset++] = '\0';
  reference_counts_[ret_val] = 1;
  hole_length -= space_needed;
  if (hole_length > 0) {
    updated_free.location = offset;
    updated_free.hole_length = hole_length;
    return ret_val;
  }
  // If we reach here, we can eliminate a hole. We will move
  // the trailing hole to this position. Unless of course, this
  // is the last position, in which case, we only do the size
  // decrement.
  if (offset < last_free_pos) {
    updated_free = free_key_list_[last_free_pos];
  }
  // special case: we are completely out of space as we just filled in
  // the last hole. Alternatively, we just allocated all of our space
  // to a single very large string.
  if (free_key_list_.size() == 1) {
    updated_free.location = max_size_;
    updated_free.hole_length = 0;
  } else {
    free_key_list_.pop_back();
  }
  return ret_val;
}

uint8_t StringAllocator::AddReference(uint16_t position) {
  uint8_t current_val = reference_counts_[position];
  if (current_val == 0 || current_val >= MAX_REFERENCE_COUNT) {
    return 0;
  }
  return ++(reference_counts_[position]);
}

uint8_t StringAllocator::DropReference(uint16_t position) {
  uint8_t current_val = reference_counts_[position];
  if (current_val == 0) {
    return 0;
  }
  current_val = --(reference_counts_[position]);
  if (current_val > 0) {
    return current_val;
  }
  // Before making a new hole, let's see if this can extend an existing
  // hole. The extension could occur because there is a hole at some pos
  // p with hole_length l such that p + l == position or it could occur
  // if there is a hole at position p such that p == position +
  // strlen(key_list_ + 1) + 1. Alternatively, the hole could occur at
  // both ends. Interestingly, it could not happen more than once on
  // either end. However, we will stop after finding one at either end
  // calling the result "good enough". Technically, this could cause
  // fragmentation, but these are expected to be used in small enough
  // structures that this should not be a huge issue.
  uint16_t new_hole_length = strlen(key_list_ + position) + 1;
  uint16_t next_hole_pos = position + new_hole_length;
  for (uint16_t i = 0; i < free_key_list_.size(); i++) {
    FreeKey& cur_free = free_key_list_[i];
    if (cur_free.location == next_hole_pos) {
      cur_free.location = position;
      cur_free.hole_length += new_hole_length;
      new_hole_length = cur_free.hole_length;
      return 0;
    }
    if (cur_free.location + cur_free.hole_length == position) {
      cur_free.hole_length = cur_free.hole_length + new_hole_length;
      position = cur_free.location;
      new_hole_length = cur_free.hole_length;
      return 0;
    }
  }

  // If we reach here, we could not re-use an existing hole, so we
  // add a new one.
  FreeKey new_hole(position, new_hole_length);
  free_key_list_.push_back(new_hole);
  return 0;
}

} // namespace collection
} // namespace acumio
