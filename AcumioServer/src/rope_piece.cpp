//============================================================================
// Name        : rope_piece.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Implementation of RZStringHash class.
//============================================================================

#include "rope_piece.h"

#include <cstring>
#include "util_constants.h"

namespace acumio {
namespace string {

RopePiece::Iterator::Iterator() : stack_(), saved_tmp_(nullptr) {}
RopePiece::Iterator::Iterator(const RopePiece* container, uint32_t index) :
    stack_(), saved_tmp_(nullptr) {
  if (container == nullptr || index >= container->length()) {
    return;
  }

  if (container->string_ != nullptr) {
    RopePiece::Iterator::StackElement elt(container, index);
    stack_.push(elt);
    return;
  }

  const RopePiece* elt_rope = container;
  uint32_t elt_index = index;
  while (elt_rope->string_ == nullptr) {
    if (elt_rope->prefix_ == nullptr) {
      RopePiece::Iterator::StackElement elt(elt_rope, 0);
      stack_.push(elt);
      elt_rope = elt_rope->suffix_.get();
    } else if (elt_rope->prefix_->length() <= elt_index) {
      // assert: elt_rope->suffix_ != nullptr.
      // Suffix contains offset into RopePiece of interest.
      uint32_t prefix_len = elt_rope->prefix_->length();
      RopePiece::Iterator::StackElement elt(elt_rope, prefix_len);
      stack_.push(elt);
      elt_rope = elt_rope->suffix_.get();
      elt_index -= prefix_len;
    } else {
      // assert: elt_rope->prefix_->length() > elt_index, so prefix
      // contains offset into RopePiece of interest.
      RopePiece::Iterator::StackElement elt(elt_rope, 0);
      stack_.push(elt);
      elt_rope = elt_rope->prefix_.get();
    }
  }

  RopePiece::Iterator::StackElement elt(elt_rope, elt_index);
  stack_.push(elt);
}

RopePiece::Iterator::Iterator(const Iterator& other) :
    stack_(other.stack_), saved_tmp_(nullptr) {}

RopePiece::Iterator::~Iterator() {}

BasicIterator<char>* RopePiece::Iterator::Clone() const {
  return new RopePiece::Iterator(*this);
}

BasicIterator<char>& RopePiece::Iterator::operator++() {
  if (stack_.empty()) {
    return *this;
  }

  RopePiece::Iterator::StackElement& top = stack_.top();
  // assert: stack_.top().rope_->string_ != nullptr
  if (top.index_ + 1 < top.rope_->length()) {
    top.index_++;
    return *this;
  }

  stack_.pop();
  if (!stack_.empty()) {
    top = stack_.top();
  }

  while (!stack_.empty() &&
         (top.index_ > 0 ||
          top.rope_->prefix_ == nullptr ||
          top.rope_->prefix_->length() == 0 ||
          top.rope_->suffix_ == nullptr ||
          top.rope_->suffix_->length() == 0)) {
    stack_.pop();
    if (!stack_.empty()) {
      top = stack_.top();
    }
  }

  if (stack_.empty()) {
    return *this;
  }

  // assert: top.rope_->prefix_->length() > 0 && top.index_ == 0
  //         top.rope_->suffix_->length() > 0
  top.index_ = top.rope_->prefix_->length();
  StackElement elt(top.rope_->suffix_.get(), 0);
  stack_.push(elt);
  while (elt.rope_->string_ == nullptr) {
    // assert: elt_.rope_->prefix_ != nullptr || elt_.rope_->suffix_ != nullptr
    // The reason we can make this assertion is that the length of the rope
    // would be 0, contradicting an earlier assumption that the rope has more
    // characters.
    elt.rope_ = (elt.rope_->prefix_.get() == nullptr ?
                 elt.rope_->suffix_.get() : elt.rope_->prefix_.get());
    stack_.push(elt);
  }

  // assert: elt.rope_->string_ != nullptr, and elt is already on the stack.

  return *this;
}

BasicIterator<char>& RopePiece::Iterator::operator--() {
  if (stack_.empty()) {
    return *this;
  }

  RopePiece::Iterator::StackElement& top = stack_.top();
  // assert: stack_.top().rope_->string_ != nullptr
  if (top.index_ > 0) {
    top.index_--;
    return *this;
  }

  stack_.pop();
  if (!stack_.empty()) {
    top = stack_.top();
  }

  while (!stack_.empty() && (top.index_ == 0)) {
    // It's possible in this manner to clear out the stack,
    // in which case, we end up going "before" the beginning of
    // the iterator, which is treated the same as the end() iterator.
    stack_.pop();
    if (!stack_.empty()) {
      top = stack_.top();
    }
  }

  if (stack_.empty()) {
    return *this;
  }

  // assert: top.index_ > 0.
  // Now, we will force it to 0, because we will push its prefix on the stack.
  top.index_ = 0;
  StackElement elt(nullptr, 0);
  elt.rope_ = top.rope_->prefix_.get();
  elt.index_ = ((elt.rope_->string_ == nullptr) ?
                (elt.rope_->prefix_.get() == nullptr ?
                 0 : elt.rope_->prefix_->length() - 1) :
                 (elt.rope_->length() - 1));
  stack_.push(elt);
  while (elt.rope_->string_ == nullptr) {
    // Inside the loop, we push the suffix for the current element onto
    // the stack. The premise being that we want the end of the current
    // rope in the stack frame. Of course, if the suffix is null, we
    // use the prefix instead.
    elt.rope_ = (elt.rope_->suffix_.get() == nullptr ?
                 elt.rope_->prefix_.get() : elt.rope_->suffix_.get());
    elt.index_ = ((elt.rope_->string_ == nullptr) ?
                  (elt.rope_->prefix_.get() == nullptr ?
                   0 : elt.rope_->prefix_->length() - 1) :
                   (elt.rope_->length() - 1));
    stack_.push(elt);
  }

  // assert: elt.rope_->string_ != nullptr, and elt is already on the stack.
  return *this;
}

BasicIterator<char>& RopePiece::Iterator::operator++(int) {
  saved_tmp_.reset(new Iterator(*this));
  (*this)++;
  return *saved_tmp_;
}

BasicIterator<char>& RopePiece::Iterator::operator--(int) {
  saved_tmp_.reset(new Iterator(*this));
  (*this)--;
  return *saved_tmp_;
}

const char& RopePiece::Iterator::operator*() const {
  if (stack_.empty()) {
    return NULL_TERMINATOR;
  }

  RopePiece::Iterator::StackElement top = stack_.top();
  // assert: top.rope_->string_ != nullptr &&
  //         top.rope_->length() > 0.
  return top.rope_->string_[top.index_];
}

const char* RopePiece::Iterator::operator->() const {
  if (stack_.empty()) {
    return &(NULL_TERMINATOR);
  }

  RopePiece::Iterator::StackElement top = stack_.top();
  // assert: top.rope_->string_ != nullptr &&
  //         top.rope_->length() > 0.
  return &(top.rope_->string_[top.index_]);
}

bool RopePiece::Iterator::operator==(const BasicIterator<char>& other) const {
  const Iterator* other_iter = dynamic_cast<const Iterator*>(&other);
  return stack_ == other_iter->stack_;
}

RopePiece::RopePiece() : string_(LETTER_STRINGS[0]), prefix_(nullptr),
    suffix_(nullptr), length_(0) {}
RopePiece::RopePiece(const char* s) : string_(s), prefix_(nullptr),
    suffix_(nullptr), length_(std::strlen(s)) {}
RopePiece::RopePiece(const std::string& s) : string_(s.c_str()),
    prefix_(nullptr), suffix_(nullptr), length_(s.length()) {}
RopePiece::RopePiece(char c) : RopePiece() {
  string_ = LETTER_STRINGS[static_cast<uint8_t>(c)];
}
// Ownership of elements of r are retained by r. Assuming lifetime of
// r exceeds lifetime of constructed RopePiece, this should not be a problem.
RopePiece::RopePiece(const RopePiece& r) : string_(r.string_),
    prefix_(r.prefix_), suffix_(r.suffix_), length_(r.length_) {}
RopePiece::RopePiece(const RopePiece* prefix, const RopePiece* suffix) :
    string_(nullptr), prefix_(prefix), suffix_(suffix), length_(0) {
  if (prefix_ != nullptr) {
    length_ += prefix_->length();
  }
  if (suffix_ != nullptr) {
    length_ += suffix_->length();
  }
}

RopePiece::RopePiece(std::shared_ptr<const RopePiece> prefix,
                     std::shared_ptr<const RopePiece> suffix) :
    string_(nullptr), prefix_(prefix), suffix_(suffix), length_(0) {
  if (prefix_ != nullptr) {
    length_ += prefix_->length();
  }
  if (suffix_ != nullptr) {
    length_ += suffix_->length();
  }
}

RopePiece::~RopePiece() {}

const RopePiece& RopePiece::operator=(const RopePiece& r) {
  string_ = r.string_;
  prefix_ = r.prefix_;
  suffix_ = r.suffix_;
  return *this;
}

// TODO: Consider getting rid of recursion to improve performance.
char RopePiece::CharAt(uint32_t position) const {
  if (position >= length_) {
    return '\0';
  }

  if (string_ != nullptr) {
    return string_[position];
  }

  // assert: string_ == nullptr.
  if (prefix_ == nullptr) {
    if (suffix_ == nullptr) {
      return '\0';
    }
    return suffix_->CharAt(position);
  }

  if (position < prefix_->length()) {
    return prefix_->CharAt(position);
  }

  if (suffix_ == nullptr) {
    return '\0';
  }

  return prefix_->CharAt(position - prefix_->length());
}

std::string RopePiece::ToString() const {
  if (string_ != nullptr) {
    return std::string(string_);
  }

  if (prefix_ == nullptr) {
    if (suffix_ == nullptr) {
      return std::string();
    }
    return suffix_->ToString();
  }

  if (suffix_ == nullptr) {
    return prefix_->ToString();
  }

  char* buffer = new char[length_ + 1];
  CopyToBuffer(buffer);
  std::string ret_val(buffer);
  delete [] buffer;
  return ret_val;
}

// TODO: Consider getting rid of recursion to improve speed.
void RopePiece::CopyToBuffer(char* buffer) const {
  if (string_ != nullptr) {
    const char* i = string_;
    while (*i != '\0') {
      *buffer++ = *i++;
    }
    return;
  }

  if (prefix_ == nullptr) {
    if (suffix_ != nullptr) {
      suffix_->CopyToBuffer(buffer);
    }
    return;
  }

  if (suffix_ == nullptr) {
    prefix_->CopyToBuffer(buffer);
    return;
  }

  prefix_->CopyToBuffer(buffer);
  suffix_->CopyToBuffer(buffer + prefix_->length()); 
}

int RopePiece::compare(const RopePiece& other) const {
  const Iterator self_end = end();
  const Iterator other_end = other.end();
  Iterator self_iter = begin();
  Iterator other_iter = other.begin();
  while (self_iter != self_end && other_iter != other_end &&
         *self_iter == *other_iter) {
    self_iter++;
    other_iter++;
  }
  if (self_iter == self_end) {
    return (other_iter == other_end) ? 0 : -1;
  }
  if (other_iter == other_end) {
    return 1;
  }
  // TODO: Double-check this. When working with UTF8 characters, how do you
  //       perform lexical comparison? char is a signed value, but we probably
  //       want to work with unsigned. In any case, this requires investigation
  //       to be sure we get the correct semantics.
  uint8_t self_char = static_cast<uint8_t>(*self_iter);
  uint8_t other_char = static_cast<uint8_t>(*other_iter);
  return self_char > other_char ?  1 : -1;
}

int RopePiece::compare(const char* other) const {
  const Iterator self_end = end();
  Iterator self_iter = begin();
  const char* other_iter = other;
  while (self_iter != self_end && *other_iter != '\0' &&
         *self_iter == *other_iter) {
    self_iter++;
    other_iter++;
  }
  if (self_iter == self_end) {
    return (other_iter == '\0') ? 0 : -1;
  }
  if (other_iter == '\0') {
    return 1;
  }
  // TODO: Double-check this. When working with UTF8 characters, how do you
  //       perform lexical comparison? char is a signed value, but we probably
  //       want to work with unsigned. In any case, this requires investigation
  //       to be sure we get the correct semantics.
  uint8_t self_char = static_cast<uint8_t>(*self_iter);
  uint8_t other_char = static_cast<uint8_t>(*other_iter);
  return self_char > other_char ?  1 : -1;
}

const RopePiece::Iterator RopePiece::begin() const {
  return RopePiece::Iterator(this, 0);
}

const RopePiece::Iterator RopePiece::end() const {
  return RopePiece::Iterator();
}

RopePiece::Iterator RopePiece::IteratorFromIndex(uint32_t index) const {
  return RopePiece::Iterator(this, index);
}

} // namespace string
} // namespace acumio
