//============================================================================
// Name        : encrypter.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Encrypter implementations.
//============================================================================

#include "encrypter.h"

namespace acumio {
namespace crypto {

EncrypterInterface::~EncrypterInterface() {}
SaltGeneratorInterface::~SaltGeneratorInterface() {}
NoOpEncrypter::~NoOpEncrypter() {}
std::string NoOpEncrypter::operator() (const std::string& password,
                                       const std::string& salt) const {
  return password;
}
DeterministicSaltGenerator::~DeterministicSaltGenerator() {}
std::string DeterministicSaltGenerator::operator() () {
  return std::to_string(value_++);
}

} // namespace crypto
} // namespace acumio
