#ifndef AcumioServer_encrypter_h
#define AcumioServer_encrypter_h
//============================================================================
// Name        : encrypter.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Provides Functor objects for doing encryption (not decryption)
//               and salt generation.
//               At the moment, only a truly brain-dead and insecure encryption
//               mechanism and salt generation is used. Later, we will use a
//               standards-based encryption and salt generator.
//
//               Note: It is a bad idea to write our own, even if we (think) we
//               know what we are doing. Handling problems such as poorly
//               defined salt mechanisms, or poorly selected algorithm
//               parameters in RSA or properly writing of the algorithm to
//               handle timing attacks - these problems are interesting, but we
//               should not reinvent the solutions.
//============================================================================

#include <string>

using namespace std;
namespace acumio {
namespace crypto {
class EncrypterInterface {
 public:
  EncrypterInterface() {}
  virtual ~EncrypterInterface();
  virtual std::string operator() (const std::string& password,
                                  const std::string& salt) const = 0;
};

class SaltGeneratorInterface {
 public:
  SaltGeneratorInterface() {}
  virtual ~SaltGeneratorInterface();
  virtual std::string operator() () = 0;
};

class NoOpEncrypter : public EncrypterInterface {
 public:
  NoOpEncrypter() : EncrypterInterface() {}
  ~NoOpEncrypter();
  std::string operator() (const std::string& password,
                          const std::string& salt) const;
};

// This class is only useful in testing. This is horrible in terms
// of Crypto.
class DeterministicSaltGenerator : public SaltGeneratorInterface {
 public:
  DeterministicSaltGenerator(uint64_t first_value) :
    SaltGeneratorInterface(), value_(first_value) {}
  ~DeterministicSaltGenerator();
  std::string operator() ();
 private:
  uint64_t value_;
};

} // namespace crypto
} // namespace acumio

#endif // AcumioServer_encrypter_h
