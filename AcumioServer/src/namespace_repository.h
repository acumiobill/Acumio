#ifndef AcumioServer_namespace_repository_h
#define AcumioServer_namespace_repository_h
//============================================================================
// Name        : namespace_repository.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Provides multi-threaded access to an in-memory
//               repository of Namespaces.
//============================================================================

#include "mem_repository.h"
#include "description.pb.h"
#include "names.pb.h"
#include <grpc++/support/status.h>

namespace acumio {

namespace model {

struct DescribedNamespace {
  Namespace name_space;
  DescriptionHistory description_history;
};

} // namespace model

// We wrap an underlying _NamespaceRepository for a couple reasons:
// First, because we want to provide cleaner APIs than are available in the
// generic MemRepository, and second, because because an inheritance model
// can actually get in our way. Composition better-than Inheritance
// (Joshua Block: Effective Java rule number 14).
class NamespaceRepository {
 public:
  typedef mem_repository::MemRepository<model::DescribedNamespace> _Repository;
  typedef _Repository::PrimaryIterator PrimaryIterator;
  typedef _Repository::SecondaryIterator SecondaryIterator;

  NamespaceRepository();
  ~NamespaceRepository();

  inline grpc::Status Add(const model::DescribedNamespace& name_space) {
    return repository_->Add(name_space);
  }

  grpc::Status Get(const std::string& full_name, model::Namespace* elt) const;

  grpc::Status GetDescribedNamespace(const std::string& full_name,
                                     model::DescribedNamespace* elt) const {
    std::unique_ptr<Comparable> key(new StringComparable(full_name));
    // TODO: Translate NOT_FOUND results to present better error message.
    return repository_->Get(key, elt);
  }

  inline grpc::Status Remove(const std::string& full_name) {
    std::unique_ptr<Comparable> key(new StringComparable(full_name));
    return repository_->Remove(key);
  }
  grpc::Status Update(const std::string& full_name,
                      const model::Namespace& name_space);

  PrimaryIterator LowerBoundByFullName(const std::string& full_name) {
    std::unique_ptr<Comparable> key(new StringComparable(full_name));
    return repository_->LowerBound(key);
  }

  SecondaryIterator LowerBoundByShortName(const std::string& short_name) {
    std::unique_ptr<Comparable> string_key(new StringComparable(short_name));
    return repository_->LowerBoundByIndex(string_key, 0);
  }

 private:
  std::unique_ptr<_Repository> repository_;
};

} // namespace acumio

#endif // AcumioServer_namespace_repository_h
