//============================================================================
// Name        : NamespaceService.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Service for Namespace operations.
//============================================================================

#include "NamespaceService.h"

#include <sstream>

namespace acumio {

namespace {
std::string ExpectedFullName(const model::QualifiedName& qualified,
                             const std::string& separator) {
  std::stringstream generated;
  generated << qualified.name_space() << separator << qualified.name();
  return generated.str();
}

// We want to make sure that before we add a Namespace, that we know
// that the full_name for it actually makes sense. We verify here
// the assertion given in the names.proto for the Namespace message.
bool NamespaceFormatConsistent(const std::string& full_name,
                               const model::QualifiedName& qualified,
                               const model::Namespace& parent) {
  if (qualified.name_space() != parent.full_name()) {
    return false;
  }
  return full_name == ExpectedFullName(qualified, parent.separator());
}

// Returns true if the provided Namespace is at least structurally
// able to represent a Namespace. There is no enforcement here that
// the namespace actually represents anything interesting, or that
// the namespace might already be in use.
bool ValidTopLevelNamespace(const model::Namespace& name_space) {
  return name_space.name().name_space().empty() &&
         name_space.name().name() == name_space.full_name();
}

// Returns true if the provided Namespace is structurally identical
// to our expectations for a Root namespace.
bool ValidRootNamespace(const model::Namespace& name_space) {
  return ValidTopLevelNamespace(name_space) &&
         name_space.full_name().empty() &&
         name_space.separator().empty();
}

bool ValidateSeparator(const model::Namespace& name_space) {
  if (ValidRootNamespace(name_space)) {
    return true;
  }

  const std::string& separator = name_space.separator();
  if (separator.empty()) {
    return false;
  }

  return name_space.name().name().find(separator) == std::string::npos;
}

} // anonymous namespace

NamespaceService::NamespaceService(
    NamespaceRepository* repository,
    ReferentialService* referential_service) :
    repository_(repository), referential_service_(referential_service) {}

NamespaceService::~NamespaceService() {}

grpc::Status NamespaceService::CreateNamespace(
    const model::Namespace& name_space,
    const model::Description& description) {
  grpc::Status result = ValidateNewNamespace(name_space);
  if (!result.ok()) {
    return result;
  }
  return repository_->AddWithDescription(name_space, description);
}

grpc::Status NamespaceService::GetJustNamespace(
    const std::string& name_space,
    model::server::GetNamespaceResponse* response) {
  return repository_->GetNamespace(name_space,
                                   response->mutable_name_space());
}

grpc::Status NamespaceService::GetNamespace(
    const std::string& name_space,
    bool include_description,
    bool include_description_history,
    model::server::GetNamespaceResponse* response) {
  if (!include_description && !include_description_history) {
    return GetJustNamespace(name_space, response);
  }

  if (include_description_history) {
    grpc::Status result = repository_->GetNamespaceAndDescriptionHistory(
        name_space,
        response->mutable_name_space(),
        response->mutable_description_history());
    if (!result.ok()) {
      return result;
    }

    if (include_description) {
      uint32_t num_versions = response->description_history().version_size();
      if (num_versions > 0) {
        response->mutable_description()->CopyFrom(
          response->description_history().version(num_versions - 1));
      }
    }
    return grpc::Status::OK;
  }

  // assert: include_description is true.
  return repository_->GetNamespaceAndDescription(
      name_space,
      response->mutable_name_space(),
      response->mutable_description());
}

grpc::Status NamespaceService::RemoveNamespace(
    const std::string& namespace_name) {
  grpc::Status check_result = ValidateNamespaceRemoval(namespace_name);
  if (!check_result.ok()) {
    return check_result;
  }
  return repository_->Remove(namespace_name);
}

grpc::Status NamespaceService::UpdateNamespace(
    const std::string& namespace_name,
    const model::Namespace& update) {
  grpc::Status check = ValidateNamespaceUpdate(namespace_name, update);
  if (check.error_message() == "ok") {
    // This condition tells us that there is no real update required.
    return grpc::Status::OK;
  }

  if (!check.ok()) {
    return check;
  }
  return repository_->UpdateNoDescription(namespace_name, update);
}

grpc::Status NamespaceService::UpdateNamespaceWithDescription(
    const std::string& namespace_name,
    const model::Namespace& update,
    const model::Description& updated_description,
    bool clear_description) {
  grpc::Status check = ValidateNamespaceUpdate(namespace_name, update);
  if (!check.ok() && check.error_message() != "ok") {
    return check;
  }

  if (check.error_message() == "ok") {
    return grpc::Status::OK;
  }

  if (clear_description) {
    return repository_->UpdateAndClearDescription(namespace_name, update);
  } 

  return repository_->UpdateWithDescription(namespace_name, update,
                                            updated_description);
}

grpc::Status NamespaceService::UpsertNamespaceDescription(
    const std::string& described, const model::Description& update,
    bool clear_description) {
  if (clear_description) {
    return repository_->ClearDescription(described);
  }

  return repository_->UpdateDescriptionOnly(described, update);
}

// Note that this does not perform a dulicate-key check. The duplicate
// key check is performed in the repository. Instead, this just validates
// that the specified namespace is well-formed, and if it specifies
// a parent namespace, that the referenced parent namespace actually
// exists.
grpc::Status NamespaceService::ValidateNewNamespace(
   const model::Namespace& name_space) {
  if (!ValidateSeparator(name_space)) {
    std::stringstream error;
    error << "Namespaces that are not the root namespace must specify a "
          << "separator. Moreover, the separator must not be a part of "
          << "the local Namespace name.";
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, error.str());
  }

  if (!ValidTopLevelNamespace(name_space)) {
    model::Namespace parent;
    grpc::Status parent_search_result =
        repository_->GetNamespace(name_space.name().name_space(), &parent);
    if (!parent_search_result.ok()) {
      return parent_search_result;
    }

    if (!NamespaceFormatConsistent(name_space.full_name(),
                                   name_space.name(),
                                   parent)) {
      std::stringstream error;
      error << "Unable to match the provided full name (\""
            << name_space.full_name()
            << "\") with the expected name based on the parent "
            << "Namespace with name (\""
            << name_space.name().name_space()
            << "\"). Expected full name was: (\""
            << ExpectedFullName(name_space.name(), parent.separator())
            << "\")";
      return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, error.str());
    }
  }

  return grpc::Status::OK;
}

grpc::Status NamespaceService::ValidateNamespaceRemoval(
    const std::string& namespace_name) {
  NamespaceRepository::PrimaryIterator it =
      repository_->LowerBoundByFullName(namespace_name);

  // First element found should be the namespace itself that we wish
  // to remove.
  // Note that the iterator "it" has as elements:
  //  pair<unique_ptr<Comparable>, Namespace> 
  // We use the Namespace's full_name attribute for comparison.
  if (it == repository_->primary_end() ||
      (*it).second.entity.full_name() != namespace_name) {
    std::stringstream error;
    error << "Unable to locate Namespace with name (\""
          << namespace_name
          << "\") for removal.";
    return grpc::Status(grpc::StatusCode::NOT_FOUND, error.str());
  }

  if (referential_service_->IsNamespaceEmpty(it->second.entity)) {
    return grpc::Status::OK;
  }

  std::stringstream error;
  error << "Unable to delete Namespace with name (\""
        << namespace_name
        << "\") since it contains elements within it.";
  return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, error.str());
}

grpc::Status NamespaceService::ValidateNamespaceUpdate(
    const std::string& namespace_name,
    const model::Namespace& update) {
  // First, check well-formedness relative to parent and having internal
  // consistency.
  grpc::Status result = ValidateNewNamespace(update);
  if (!result.ok()) {
    return result;
  }

  // Now, find the existing element - we need to see if the update operation
  // will amount to a removal of the element so far as it's children are
  // concerned. If we are changing the separator then child elements will
  // need to be removed first, as otherwise, their calculated fullnames
  // will be incorrect.
  NamespaceRepository::PrimaryIterator it =
      repository_->LowerBoundByFullName(namespace_name);

  // Similar to the case of Namespace Removal, we expect that the found
  // result should match an existing Namespace with the provided
  // namespace_name.
  if (it == repository_->primary_end()) {
    std::stringstream error;
    error << "Unable to locate Namespace with name (\""
          << namespace_name
          << "\") for update.";
    return grpc::Status(grpc::StatusCode::NOT_FOUND, error.str());
  }

  const model::Namespace& found = it->second.entity;

  if (found.full_name() != namespace_name) {
    std::stringstream error;
    error << "Found a Namespace using namespace_name = (\""
          << namespace_name
          << "\"), but its full_name (\""
          << found.full_name()
          << "\") does not appear to match. "
          << "This is a sign of data corruption.";
    return grpc::Status(grpc::StatusCode::INTERNAL, error.str());
  }

  // Now, having found the Namespace, we check if any of the attributes
  // that could generate an effective Namespace-delete event have changed.
  if (update.full_name() == namespace_name &&
      update.separator() == found.separator() &&
      update.name().name() == found.name().name()) {
    // The only other attribute to check is the "is_repository_name"
    // flag. If these are identical, we have no work to do in the update.
    if (found.is_repository_name() == update.is_repository_name()) {
      // This is a bit of a hack: we don't have a way of extending the
      // grpc::StatusCode codes, but if we did, what we would want to say
      // here is that no operation is required, and it's "ok" to return
      // "ok". We will check the error string in the return process.
      return grpc::Status(grpc::StatusCode::CANCELLED, "ok");
    }
    // In this case, there is no "delete" operation to check, so we just
    // allow the update to occur.
    return grpc::Status::OK;
  }

  if (referential_service_->IsNamespaceEmpty(it->second.entity)) {
    return grpc::Status::OK;
  }
  std::stringstream error;
  error << "Unable to rename Namespace (\""
        << namespace_name
        << "\") since it is not empty.";
  return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, error.str());
}

} // namespace acumio
