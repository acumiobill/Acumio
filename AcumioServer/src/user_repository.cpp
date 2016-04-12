//============================================================================
// Name        : user_repository.cc
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Provides multi-threaded access to an in-memory
//               repository.
//============================================================================

#include "user_repository.h"

namespace acumio {

namespace {
typedef mem_repository::KeyExtractorInterface<FullUser> _UserExtractor;
class NameExtractor : public _UserExtractor {
 public:
  NameExtractor() : _UserExtractor() {}
  ~NameExtractor() {}
  inline std::unique_ptr<Comparable> GetKey(const FullUser& element) const {
    std::unique_ptr<Comparable> key(
        new StringComparable(element.user().name()));
    return key;
  }
};

class ContactEmailExtractor : public _UserExtractor {
 public:
  ContactEmailExtractor() : _UserExtractor() {}
  ~ContactEmailExtractor() {}
  inline std::unique_ptr<Comparable> GetKey(const FullUser& element) const {
    std::unique_ptr<Comparable> key(
        new StringComparable(element.user().contact_email()));
    return key;
  }
};

} // anonymous namespace

FullUser::~FullUser() {}

UserRepository::UserRepository() : repository_(nullptr) {
  std::unique_ptr<_UserExtractor> main_extractor(new NameExtractor());
  std::unique_ptr<_UserExtractor> email_extractor(new ContactEmailExtractor());
  std::vector<std::unique_ptr<_UserExtractor>> additional_extractors;
  additional_extractors.push_back(std::move(email_extractor));
  repository_.reset(new _UserRepository(std::move(main_extractor),
                                        &additional_extractors));
}

UserRepository::~UserRepository() {}
} // namespace acumio
