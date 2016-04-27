//============================================================================
// Name        : referential_service.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Shared services for performing referential integrity checks.
//============================================================================

#include "referential_service.h"

#include <grpc++/grpc++.h>
#include "dataset_repository.h"
#include "names.pb.h"
#include "namespace_repository.h"
#include "repository_repository.h"

namespace acumio {
ReferentialService::ReferentialService(
    NamespaceRepository* namespace_repository,
    DatasetRepository* dataset_repository,
    RepositoryRepository* repository_repository) :
    namespace_repository_(namespace_repository),
    dataset_repository_(dataset_repository),
    repository_repository_(repository_repository) {}

ReferentialService::~ReferentialService() {}

grpc::Status ReferentialService::CreateOrAssociateNamespace(
    const model::Namespace& found_parent,
    const std::string& name,
    const std::string& separator,
    const model::Description& description) {
  std::stringstream full_name;
  full_name << found_parent.full_name() << found_parent.separator() << name;
  model::Namespace found_namespace;
  model::Description found_description;
  grpc::Status find_result = GetNamespaceAndDescription(full_name.str(),
                                                        &found_namespace,
                                                        &found_description);
  if (find_result.ok()) {
    return AssociateNamespace(found_namespace, found_description, description,
                              separator);
  } else if (find_result.error_code() == grpc::StatusCode::NOT_FOUND) {
    return CreateAssociatedNamespace(full_name.str(), found_parent.full_name(),
                                     name, separator, description);
  }
  // if find_result.error_code() is something other than OK or NOT_FOUND, we
  // just return the error.
  return find_result;
}

grpc::Status ReferentialService::GetParentNamespace(
    const model::QualifiedName& child_name,
    const char* child_type,
    model::Namespace* parent) const {
  grpc::Status result = namespace_repository_->GetNamespace(
      child_name.name_space(), parent);
  if (!result.ok() && result.error_code() == grpc::StatusCode::NOT_FOUND) {
    std::stringstream error;
    error << "Unable to get parent Namespace (\""
          << child_name.name_space()
          << "\") for "
          << child_type
          << " with name: (\""
          << child_name.name()
          << "\"). Cannot create "
          << child_type
          << " if parent namespace does not exist.";
    return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, error.str());
  }

  return result;
}

grpc::Status ReferentialService::GetNamespace(
    const std::string& namespace_name,
    model::Namespace* name_space) const {
  return namespace_repository_->GetNamespace(namespace_name, name_space);
}

grpc::Status ReferentialService::GetNamespaceAndDescription(
    const std::string& namespace_name,
    model::Namespace* name_space,
    model::Description* description) const {
  return namespace_repository_->GetNamespaceAndDescription(
      namespace_name, name_space, description);
}

grpc::Status ReferentialService::GetNamespaceUsingParent(
    const model::Namespace& parent,
    const std::string& name,
    model::Namespace* name_space) {
  std::stringstream new_fullname;
  new_fullname << parent.full_name()
               << parent.separator()
               << name;
  return namespace_repository_->GetNamespace(new_fullname.str(), name_space);
}

bool ReferentialService::IsNamespaceEmpty(
    const model::Namespace& name_space) const {
  return (!NamespaceContainsNamespace(name_space) &&
          !NamespaceContainsRepository(name_space) &&
          !NamespaceContainsDataset(name_space));
}

grpc::Status ReferentialService::RemoveNamespace(
    const std::string& namespace_name) {
  return namespace_repository_->Remove(namespace_name);
}

//********************* PRIVATE METHODS ***************************/
grpc::Status ReferentialService::AssociateNamespace(
    const model::Namespace& found_namespace,
    const model::Description& found_description,
    const model::Description& update_description,
    const std::string& desired_separator) {
  if (found_namespace.separator() == desired_separator &&
      found_namespace.is_repository_name()) {
    if (found_description.contents() == update_description.contents() ||
        update_description.contents().empty()) {
      // No update required.
      return grpc::Status::OK;
    }
    return namespace_repository_->UpdateDescriptionOnly(
        found_namespace.full_name(), update_description);
  }

  // At this point, we know that *something* about the namespace needs to be
  // updated. It's guaranteed that the is_repository_name needs to be set to
  // true (well, okay, it might already be true, but we know the final value
  // should be true). We still need to be concerned still about the separator.
  model::Namespace update_namespace(found_namespace);
  update_namespace.set_is_repository_name(true);

  if (found_namespace.separator() != desired_separator) {
    if (NamespaceContainsNamespace(found_namespace)) {
      std::stringstream error;
      error << "Unable to associate namespace (\""
            << found_namespace.full_name()
            << "\") with the separator (\""
            << desired_separator
            << "\") since it would imply changing its child namespaces.";
      return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, error.str());
    }
    update_namespace.set_separator(desired_separator);
  }
  if (found_description.contents() == update_description.contents() ||
      update_description.contents().empty()) {
    return namespace_repository_->UpdateNoDescription(
        update_namespace.full_name(), update_namespace);
  }
  return namespace_repository_->UpdateWithDescription(
      update_namespace.full_name(), update_namespace, update_description);
}

grpc::Status ReferentialService::CreateAssociatedNamespace(
    const std::string& full_name,
    const std::string& parent_full_name,
    const std::string& name,
    const std::string& separator,
    const model::Description& desc) {
  model::Namespace new_namespace;
  new_namespace.set_full_name(full_name);
  model::QualifiedName*  new_qualified_name = new_namespace.mutable_name();
  new_qualified_name->set_name_space(parent_full_name);
  new_qualified_name->set_name(name);
  new_namespace.set_separator(separator);
  new_namespace.set_is_repository_name(true);
  return namespace_repository_->AddWithDescription(new_namespace, desc);
}

bool ReferentialService::NamespaceContainsDataset(
    const model::Namespace& name_space) const {
  DatasetRepository::SecondaryIterator dataset_iter =
      dataset_repository_->LowerBoundByNamespace(name_space.full_name());

  if (dataset_iter == dataset_repository_->namespace_iter_end()) {
    // In this case, there are no Datasets where the namespace lookup
    // comes after or matches the desired dataset.
    return false;
  }

  // The found Dataset will either be from a Namespace matching our
  // target Namespace, or it will be one that comes after it.
  // We will check the key: if the key matches our desired target, we
  // have found a Dataset looked up by its Namespace where the Namespace
  // name matches our target name. Otherwise, no such dataset exists.
  return dataset_iter->first->to_string() == name_space.full_name();
}

bool ReferentialService::NamespaceContainsNamespace(
    const model::Namespace& name_space) const {
  std::string name_plus_separator =
      name_space.full_name() + name_space.separator();

  NamespaceRepository::PrimaryIterator it =
      namespace_repository_->LowerBoundByFullName(name_plus_separator);
  // LowerBound is guaranteed to point to first element where key is
  // *not before* name_plus_separator. Assuming separator contains a string
  // that is not a valid portion of an identifier for a Namespace
  // if we find anything with the prefix name_plus_separator, we should
  // have found our namespace. We verify by looking at the parent name_space
  // of the Namespace that we find and comparing it with our expected
  // name.
  // TODO: Consider adding an index to NamespaceRepository with lookup by
  // parent namespace. If we had that, the lookup process would be cleaner.
  // In addition, the operations within the Namespace service *might* be
  // more effective as well (the latter idea is a bit uncertain, but if it
  // turns out to be true, we should make this change. If it is not true,
  // we should keep things the way they are).
  return (it != namespace_repository_->primary_end() &&
          it->second.entity.name().name_space() == name_space.full_name());
}

bool ReferentialService::NamespaceContainsRepository(
    const model::Namespace& name_space) const {
  RepositoryRepository::SecondaryIterator repository_iter =
      repository_repository_->LowerBoundByNamespace(name_space.full_name());
  if (repository_iter == repository_repository_->namespace_end()) {
    // In this case, there are no Repositories where the namespace lookup
    // comes after or matches the desired dataset.
    return false;
  }

  // The found Repository will either be from a Namespace matching our
  // target Namespace, or it will be one that comes after it.
  // We will check the key: if the key matches our desired target, we
  // have found a Repository looked up by its containing Namespace where the
  // Namespace name matches our target name. Otherwise, no such repository
  // exists.
  return repository_iter->first->to_string() == name_space.full_name();
}
} // namespace acumio

