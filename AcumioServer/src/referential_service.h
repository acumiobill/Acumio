#ifndef AcumioServer_referential_service_h
#define AcumioServer_referential_service_h
//============================================================================
// Name        : referential_service.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Shared services for performing referential validity checks.
//============================================================================

#include <grpc++/grpc++.h>
#include "names.pb.h"
#include "dataset_repository.h"
#include "namespace_repository.h"
#include "repository_repository.h"

namespace acumio {
class ReferentialService {
 public:
  // Arguments are the various repositories. The memory space for the
  // arguments is to be owned by the clients, and the lifetime should
  // exceed that of the ReferentialService.
  ReferentialService(NamespaceRepository* namespace_repository,
                     DatasetRepository* dataset_repository,
                     RepositoryRepository* repository_repository);
  ~ReferentialService();

  // We want to either Create the namespace described as
  // found_parent + separator + name, or if it already exists,
  // we want to specify that it is associated with a Repository.
  // It should be already guaranteed by contet that found_parent
  // already exists.
  grpc::Status CreateOrAssociateNamespace(
      const model::Namespace& found_parent,
      const std::string& name,
      const std::string& separator,
      const model::Description& description);

  // Updates the input Namespace to set as not associated with a repository.
  // The context should identify that this is a "safe" operation - such as
  // making sure the provided Namespace actually exists.
  inline grpc::Status DisassociateNamespace(model::Namespace& name_space) {
    name_space.set_is_repository_name(false);
    return namespace_repository_->UpdateNoDescription(name_space.full_name(),
                                                      name_space);
  }

  // Retrieves Namespace corresponding to child_name.name_space().
  // If unable to find the parent namespace, we generate an error message
  // using the child_type parameter. the child_type should be something
  // like "repository" or "dataset" (see model_constants.h).
  grpc::Status GetParentNamespace(const model::QualifiedName& child_name,
                                  const char* child_type,
                                  model::Namespace* parent) const;

  grpc::Status GetNamespace(const std::string& namespace_name,
                            model::Namespace* name_space) const;

  grpc::Status GetNamespaceAndDescription(
      const std::string& namespace_name,
      model::Namespace* name_space,
      model::Description* description) const;

  grpc::Status GetNamespaceUsingParent(const model::Namespace& parent,
                                       const std::string& name,
                                       model::Namespace* name_space);

  bool IsNamespaceEmpty(const model::Namespace& name_space) const;

  // Removes the given Namespace -- assumes that it has already been
  // verified that te Namespace is safe to remove, and that this is
  // happening as a result of a Cascade-style delete (e.g., from
  // removal of Repository with the same name).
  grpc::Status RemoveNamespace(const std::string& namespace_name);

  inline grpc::Status RemoveOrDisassociateNamespace(
      model::Namespace name_space, bool is_empty) {
    return is_empty ? RemoveNamespace(name_space.full_name()) :
                      DisassociateNamespace(name_space);
  }

 private:
  // We will update the namespace associated with found_namespace to have
  // it reflect these updates: First, it must be set as associated with a
  // repository. Second, it must have the separator set to desired_separator.
  // If changing the separator causes child elements to be modified, this
  // generates a FAILED_PRECONDITION error. To be clear: the found_namespace
  // is a copy of the namespace we want to update; not a reference to the
  // actual namespace to be updated.
  grpc::Status AssociateNamespace(const model::Namespace& found_namespace,
                                  const model::Description& found_description,
                                  const model::Description& update_description,
                                  const std::string& desired_separator);

  // We will create a Namespace with the given attributes and with
  // set_is_repository_name set to true. We should be guaranteed that the
  // given namespace does not already exist.
  grpc::Status CreateAssociatedNamespace(const std::string& full_name,
                                         const std::string& parent_full_name,
                                         const std::string& name,
                                         const std::string& separator,
                                         const model::Description& desc);
  bool NamespaceContainsDataset(const model::Namespace& name_space) const;
  bool NamespaceContainsNamespace(const model::Namespace& name_space) const;
  bool NamespaceContainsRepository(const model::Namespace& name_space) const;
  NamespaceRepository* namespace_repository_;
  DatasetRepository* dataset_repository_;
  RepositoryRepository* repository_repository_;
}; // end class NamespaceService
} // namespace acumio
#endif // AcumioServer_referential_service_h
