//============================================================================
// Name        : RepositoryService.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Service for Repository operations.
//============================================================================

#include "RepositoryService.h"
#include "referential_service.h"
#include "repository_repository.h"

namespace acumio {

// Organization of APIs:
// Constructor,
// Destructor,
// Public Methods (alphabetically)
// Private Methods (alphabetically)

RepositoryService::RepositoryService(
    RepositoryRepository* repository,
    ReferentialService* referential_service) :
    repository_(repository), referential_service_(referential_service) {}

RepositoryService::~RepositoryService() {}

grpc::Status RepositoryService::CreateRepository(
    const model::Repository& repository,
    const model::Description& description,
    bool create_or_associate_namespace,
    const std::string& namespace_separator) {
  model::Namespace parent_namespace;
  grpc::Status check = referential_service_->GetParentNamespace(
      repository.name(), model::REPOSITORY, &parent_namespace);
  if (!check.ok()) {
    return check;
  }

  check = ValidateWellFormedRepository(repository);
  if (!check.ok()) {
    return check;
  }
  if (create_or_associate_namespace) {
    check = referential_service_->CreateOrAssociateNamespace(
        parent_namespace, repository.name().name(), namespace_separator,
        description);
    if (!check.ok()) {
      return check;
    }
  } else {
    check = VerifyAssociatedNamespaceExists(parent_namespace,
                                            repository.name());
    if (!check.ok()) {
      return check;
    }
  }

  return repository_->AddWithDescription(repository, description);
}

grpc::Status RepositoryService::GetRepository(
    const model::QualifiedName& repository_name,
    bool include_description,
    bool include_description_history,
    model::server::GetRepositoryResponse* response) {
  if (!include_description && !include_description_history) {
    return GetJustRepository(repository_name, response);
  }

  if (include_description_history) {
    grpc::Status result = repository_->GetRepositoryAndDescriptionHistory(
        repository_name,
        response->mutable_repository(),
        response->mutable_description_history());
    if (!result.ok()) {
      return result;
    }

    // If we want descriptions and description history, we will just get
    // the descriptions from the description history.
    if (include_description) {
      uint32_t num_versions = response->description_history().version_size();
      if (num_versions > 0) {
        response->mutable_description()->CopyFrom(
          response->description_history().version(num_versions - 1));
      }
    }
    return grpc::Status::OK;
  }

  // assert: include_description && !include_description_history
  return repository_->GetRepositoryAndDescription(
      repository_name,
      response->mutable_repository(),
      response->mutable_description());
}

grpc::Status RepositoryService::ListRepositories(
    uint32_t list_max,
    const model::QualifiedName& start_after_name,
    bool include_descriptions,
    model::server::ListRepositoriesResponse* response) {
  if (list_max == 0) {
    // Kind of silly, but if the max desired is 0, just return immediately.
    return grpc::Status::OK;
  }

  RepositoryRepository::PrimaryIterator it = IteratorStart(start_after_name);

  if (include_descriptions) {
    return ListRepositoriesAndDescriptions(it, list_max, response);
  }
  
  return ListJustRepositories(it, list_max, response);
}

grpc::Status RepositoryService::RemoveRepository(
    const model::QualifiedName& repository_name,
    bool force,
    bool remove_or_disassociate_namespace) {
  model::Namespace parent_namespace;
  grpc::Status check = referential_service_->GetParentNamespace(
      repository_name, model::REPOSITORY, &parent_namespace);
  if (!check.ok()) {
    return check;
  }
  std::stringstream namespace_fullname;
  namespace_fullname << parent_namespace.full_name()
                     << parent_namespace.separator()
                     << repository_name.name();

  model::Namespace name_space;
  check = referential_service_->GetNamespace(namespace_fullname.str(),
                                             &name_space);
  if (!check.ok()) {
    return check;
  }

  bool is_empty = referential_service_->IsNamespaceEmpty(name_space);

  if (!is_empty) {
    if (force) {
      return RemoveRepositoryWithForce(repository_name, name_space,
                                       remove_or_disassociate_namespace);
    } else {
      std::stringstream error;
      error << "Unable to remove Repository since it has registered elements "
            << "in the Acumio Server.";
      return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, error.str());
    }
  }

  // We can be sure here that the associated Namespace is empty.
  check = repository_->Remove(repository_name);
  if (!check.ok()) {
    return check;
  }

  if (remove_or_disassociate_namespace) {
    return referential_service_->RemoveNamespace(name_space.full_name());
  }

  return grpc::Status::OK;
}

grpc::Status RepositoryService::UpdateRepository(
    const model::QualifiedName& name,
    const model::Repository& update,
    bool force) {
  grpc::Status check = ValidateWellFormedRepository(update);
  if (!check.ok()) {
    return check;
  }
  
  if (name.name_space() == update.name().name_space() &&
      name.name() == update.name().name()) {
    // Alright, this is the easy path: No invalidation of prior Namespace
    // information or disassociation with prior Namespaces required: we
    // just update the Repository directly and we are done.
    return repository_->UpdateNoDescription(name, update);
  }

  // If we reach here, we are fundamentally changing the name of the
  // repository, hence the namespace. We need to first check if we can
  // find if the current namespace has children.
  model::Namespace current_namespace_parent;
  check = referential_service_->GetParentNamespace(
      name, model::REPOSITORY, &current_namespace_parent);
  if (!check.ok()) {
    return check;
  }
  model::Namespace current_namespace;
  std::stringstream current_fullname;
  current_fullname << current_namespace_parent.full_name()
                   << current_namespace_parent.separator()
                   << name.name();
  check = referential_service_->GetNamespace(current_fullname.str(),
                                             &current_namespace);
  if (!check.ok()) {
    return check;
  }
  bool is_empty = referential_service_->IsNamespaceEmpty(current_namespace);

  if (!force && !is_empty) {
    std::stringstream error;
    error << "Unable to rename Repository with name ((\""
          << current_fullname.str()
          << "\") since it contains registered elements.";
    return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, error.str());
  }

  // We now know the disposition of the old namespace: either we are 
  // disassociating it or we are not. However, we need to associate or
  // create the new namespace before we proceed.
  model::Namespace new_namespace_parent;
  check = referential_service_->GetParentNamespace(
      update.name(), model::REPOSITORY, &new_namespace_parent);
  if (!check.ok()) {
    return check;
  }
  model::Namespace new_namespace;
  std::stringstream new_fullname;
  new_fullname << new_namespace_parent.full_name()
               << new_namespace_parent.separator()
               << update.name().name();
  check = referential_service_->GetNamespace(new_fullname.str(),
                                             &new_namespace);
  bool new_namespace_exists = check.ok();
  // If any error is found other than NOT_FOUND, just return it. We
  // might expect NOT_FOUND if there is no namespace with the new name.
  // In fact, this should be regarded as the "typical" path.
  if (!check.ok() && check.error_code() != grpc::StatusCode::NOT_FOUND) {
    return check;
  }

  // We will first touch the new namespace before migrating off the old
  // namespace. If errors occur part way through, we will be least hurt
  // by it in that case.
  if (new_namespace_exists && new_namespace.is_repository_name()) {
    // Let's check if there is already a Repository with the given name.
    // We basically need to validate everything before we execute in order
    // to avoid getting into a bad state.
    model::Repository pre_existing_repository;
    check = repository_->GetRepository(new_namespace.name(), 
                                       &pre_existing_repository);
    if (check.ok()) {
      // If results were "ok", it means there is already a Repository with
      // the new name. Following through would imply a collision.
      std::stringstream error;
      error << "Unable to rename repository (\""
            << current_fullname.str()
            << "\") to (\""
            << new_namespace.full_name()
            << "\"). There is already a repository with that name.";
      return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, error.str());
    }
  }
  // TODO: Consider trying to use existing description in the event that
  // we are creating a new Namespace in this update process.
  model::Description desc;
  check = referential_service_->CreateOrAssociateNamespace(
      new_namespace_parent, update.name().name(), current_namespace.separator(),
      desc);
  if (!check.ok()) {
    return check;
  }

  check = repository_->UpdateNoDescription(name, update);
  if (!check.ok()) {
    return check;
  }

  if (force && is_empty) {
    return referential_service_->RemoveNamespace(current_namespace.full_name());
  } else if (force && !is_empty) {
    return referential_service_->DisassociateNamespace(current_namespace);
  }
  return grpc::Status::OK;
}

grpc::Status RepositoryService::UpdateRepositoryWithDescription(
    const model::QualifiedName& name,
    const model::Repository& update,
    const model::Description& updated_description,
    bool clear_description,
    bool force) {
  grpc::Status check = ValidateWellFormedRepository(update);
  if (!check.ok()) {
    return check;
  }
  
  if (name.name_space() == update.name().name_space() &&
      name.name() == update.name().name()) {
    // Alright, this is the easy path: No invalidation of prior Namespace
    // information or disassociation with prior Namespaces required: we
    // just update the Repository directly and we are done.
    return repository_->UpdateNoDescription(name, update);
  }

  // If we reach here, we are fundamentally changing the name of the
  // repository, hence the namespace. We need to first check if we can
  // find if the current namespace has children.
  model::Namespace current_namespace_parent;
  check = referential_service_->GetParentNamespace(
      name, model::REPOSITORY, &current_namespace_parent);
  if (!check.ok()) {
    return check;
  }
  model::Namespace current_namespace;
  check = referential_service_->GetNamespaceUsingParent(
      current_namespace_parent, name.name(), &current_namespace);
  if (!check.ok()) {
    return check;
  }
  bool is_empty = referential_service_->IsNamespaceEmpty(current_namespace);

  if (!force && !is_empty) {
    std::stringstream error;
    error << "Unable to rename Repository with name ((\""
          << current_namespace.full_name()
          << "\") since it contains registered elements.";
    return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, error.str());
  }

  // We now know the disposition of the old namespace: either we are 
  // disassociating it or we are not. However, we need to associate or
  // create the new namespace before we proceed.
  model::Namespace new_namespace_parent;
  check = referential_service_->GetParentNamespace(update.name(),
      model::REPOSITORY, &new_namespace_parent);
  if (!check.ok()) {
    return check;
  }
  model::Namespace new_namespace;
  check = referential_service_->GetNamespaceUsingParent(
      new_namespace_parent, update.name().name(), &new_namespace);
  bool new_namespace_exists = check.ok();
  // If any error is found other than NOT_FOUND, just return it. We
  // might expect NOT_FOUND if there is no namespace with the new name.
  // In fact, this should be regarded as the "typical" path.
  if (!check.ok() && check.error_code() != grpc::StatusCode::NOT_FOUND) {
    return check;
  }

  // We will first touch the new namespace before migrating off the old
  // namespace. If errors occur part way through, we will be least hurt
  // by it in that case.
  if (new_namespace_exists && new_namespace.is_repository_name()) {
    // Let's check if there is already a Repository with the given name.
    // We basically need to validate everything before we execute in order
    // to avoid getting into a bad state.
    model::Repository pre_existing_repository;
    check = repository_->GetRepository(new_namespace.name(), 
                                       &pre_existing_repository);
    if (check.ok()) {
      // If results were "ok", it means there is already a Repository with
      // the new name. Following through would imply a collision.
      std::stringstream error;
      error << "Unable to rename repository (\""
            << current_namespace.full_name()
            << "\") to (\""
            << new_namespace.full_name()
            << "\"). There is already a repository with that name.";
      return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, error.str());
    } else if (check.error_code() != grpc::StatusCode::NOT_FOUND) {
      // An error other than 'NOT_FOUND' should be returned as an error.
      // NOT_FOUND is the expected condition, and generates no error.
      return check;
    }
  }
  model::Description desc;
  check = referential_service_->CreateOrAssociateNamespace(
      new_namespace_parent, update.name().name(), current_namespace.separator(),
      (clear_description ? desc : updated_description));
  if (!check.ok()) {
    return check;
  }

  check = (clear_description ?
           repository_->UpdateAndClearDescription(name, update) :
           repository_->UpdateWithDescription(name, update,
                                              updated_description));
  if (!check.ok()) {
    return check;
  }

  if (force) {
    return referential_service_->RemoveOrDisassociateNamespace(
        current_namespace, is_empty);
  }

  return grpc::Status::OK;
}

grpc::Status RepositoryService::UpsertRepositoryDescription(
    const model::QualifiedName& repository_name,
    const model::Description& update,
    bool clear_description) {
  if (clear_description) {
    return repository_->ClearDescription(repository_name);
  }

  return repository_->UpdateDescriptionOnly(repository_name,
                                            update);
}

// PRIVATE METHODS
grpc::Status RepositoryService::GetJustRepository(
    const model::QualifiedName& repository_name,
    model::server::GetRepositoryResponse* response) {
  return repository_->GetRepository(repository_name,
                                    response->mutable_repository());
}

RepositoryRepository::PrimaryIterator RepositoryService::IteratorStart(
    const model::QualifiedName& start_after_name) const {
  RepositoryRepository::PrimaryIterator start = repository_->primary_begin();

  if (start_after_name.name_space().empty() &&
      start_after_name.name().empty()) {
    return start;
  }

  const model::QualifiedName& first_found = (start->second).entity.name();
  int name_space_compare =
      start_after_name.name_space().compare(first_found.name_space());
  if (name_space_compare < 0) {
    return start;
  }
  if (name_space_compare == 0) {
    int name_compare = start_after_name.name().compare(first_found.name());
    if (name_compare < 0) {
      return start;
    }
    if (name_compare == 0) {
      RepositoryRepository::PrimaryIterator ret_val = start++;
      return ret_val;
    }
  }

  RepositoryRepository::PrimaryIterator it =
      repository_->LowerBoundByFullName(start_after_name);
  if (it == repository_->primary_end()) {
    return it;
  }
  // We are supposed to start *after* the indicated element. If we get a
  // lower bound that equals the indicated element, we need to increment
  // the iterator first.
  const model::QualifiedName& found = (it->second).entity.name();
  if (start_after_name.name_space() == found.name_space() &&
      start_after_name.name() == found.name()) {
    it++;
  }
  return it;
}

grpc::Status RepositoryService::ListJustRepositories(
    RepositoryRepository::PrimaryIterator it,
    uint32_t list_max,
    model::server::ListRepositoriesResponse* response) const {
  uint32_t added_count = 0;
  RepositoryRepository::PrimaryIterator end_iter = repository_->primary_end();
  while (added_count < list_max && it != end_iter) {
    response->add_repository()->CopyFrom(it->second.entity);
    added_count++;
    it++;
  }

  if (it != end_iter) {
    response->set_more_results(true);
  }

  return grpc::Status::OK;
}

grpc::Status RepositoryService::ListRepositoriesAndDescriptions(
    RepositoryRepository::PrimaryIterator it,
    uint32_t list_max,
    model::server::ListRepositoriesResponse* response) const {
  uint32_t added_count = 0;
  RepositoryRepository::PrimaryIterator end_iter = repository_->primary_end();
  while (added_count < list_max && it != end_iter) {
    response->add_repository()->CopyFrom(it->second.entity);
    const model::DescriptionHistory& found_history =
        it->second.description_history;
    model::Description* added_description = response->add_description();
    int num_versions = found_history.version_size();
    if (num_versions > 0) {
      added_description->CopyFrom(found_history.version(num_versions - 1));
    }
    added_count++;
    it++;
  }

  if (it != end_iter) {
    response->set_more_results(true);
  }

  return grpc::Status::OK;
}

grpc::Status RepositoryService::RemoveRepositoryWithForce(
    const model::QualifiedName& repository_name,
    model::Namespace& name_space,
    bool remove_or_disassociate_namespace) {
  grpc::Status check = repository_->Remove(repository_name);
  if (!check.ok()) {
    return check;
  }

  if (remove_or_disassociate_namespace) {
    return referential_service_->DisassociateNamespace(name_space);
  }
  return grpc::Status::OK;
}

grpc::Status RepositoryService::ValidateWellFormedRepository(
    const model::Repository& repository) const {
  // TODO: Later, we will want to validate Repositories based on type
  //       of repository and to make sure that connect_config is
  //       well-formed. At the moment, this will suffice.
  //       The expectation is that we will use GetParentNamespace earlier
  //       in the validation process to verify referential integrity.
  return grpc::Status::OK;
}

grpc::Status RepositoryService::VerifyAssociatedNamespaceExists(
    const model::Namespace& parent,
    const model::QualifiedName& repository_name) const {
  std::stringstream namespace_fullname;
  namespace_fullname << parent.full_name()
                     << parent.separator()
                     << repository_name.name();

  model::Namespace name_space;
  grpc::Status check = referential_service_->GetNamespace(
      namespace_fullname.str(), &name_space);

  if (!check.ok()) {
    if (check.error_code() == grpc::StatusCode::NOT_FOUND) {
      std::stringstream error;
      error << "Unable to create repository with full-name: (\""
            << namespace_fullname.str()
            << "\") since there is no associated namespace for that "
            << "repository.";
      return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                          error.str());
    }
    return check;
  }

  if (!name_space.is_repository_name()) {
    std::stringstream error;
    error << "Unable to create repository with full-name: (\""
          << namespace_fullname.str()
          << "\") since the indicated namespace is not marked "
          << "as associated with a repository.";
  }

  return grpc::Status::OK;
}

} // namespace acumio

