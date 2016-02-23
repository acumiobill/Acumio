//============================================================================
// Name        : user_repository.cc
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Provides multi-threaded access to an in-memory
//               repository.
//============================================================================

#include "user_repository.h"

namespace acumio {
FullUser::~FullUser() {}

NameExtractor::~NameExtractor() {}

UserRepository::UserRepository() : repository_(nullptr) {
  std::unique_ptr<_UserExtractor> main_extractor(new NameExtractor());
  std::vector<std::unique_ptr<_UserExtractor>> additional_extractors;
  repository_.reset(new _UserRepository(std::move(main_extractor),
                                        &additional_extractors));
}

UserRepository::~UserRepository() {}
} // namespace acumio
