//============================================================================
// Name        : RepositoryService.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Service for Repository operations.
//============================================================================

#include "RepositoryService.h"
#include "namespace_repository.h"
#include "repository_repository.h"

namespace acumio {

RepositoryService::RepositoryService(
    std::shared_ptr<RepositoryRepository> repository,
    std::shared_ptr<NamespaceRepository> namespace_repository) :
    repository_(repository), namespace_repository_(namespace_repository) {}

RepositoryService::~RepositoryService() {}

grpc::Status RepositoryService::CreateRepository(
    const model::Repository& repository,
    const model::Description& description) {
  grpc::Status check = ValidateNewRepository(repository);
  if (!check.ok()) {
    return check;
  }
  return repository_->AddWithDescription(repository, description);
}

grpc::Status RepositoryService::GetJustRepository(
    const model::QualifiedName& repository_name,
    model::Repository_Type repository_type,
    model::server::GetRepositoryResponse* response) {
  return repository_->GetRepository(repository_name,
                                    repository_type,
                                    response->mutable_repository());
}

grpc::Status RepositoryService::GetRepository(
    const model::QualifiedName& repository_name,
    model::Repository_Type type,
    bool include_description,
    bool include_description_history,
    model::server::GetRepositoryResponse* response) {
  if (!include_description && !include_description_history) {
    return GetJustRepository(repository_name, type, response);
  }

  if (include_description_history) {
    grpc::Status result = repository_->GetRepositoryAndDescriptionHistory(
        repository_name,
        type,
        response->mutable_repository(),
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
  return repository_->GetRepositoryAndDescription(
      repository_name,
      type,
      response->mutable_repository(),
      response->mutable_description());
}

RepositoryRepository::PrimaryIterator RepositoryService::IteratorStart(
    const model::QualifiedName& start_after_name,
    model::Repository_Type start_after_type) const {
  RepositoryRepository::PrimaryIterator start = repository_->primary_begin();

  if (start_after_name.name_space().empty() &&
      start_after_name.name().empty() &&
      start_after_type == model::Repository_Type_UNKNOWN) {
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
      int32_t left_type = (int32_t) start_after_type;
      int32_t right_type = (int32_t) (start->second).entity.type();
      if (left_type < right_type) {
        return start;
      }
      if (left_type == right_type) {
        RepositoryRepository::PrimaryIterator ret_val = start++;
        return ret_val;
      }
    }
  }

  RepositoryRepository::PrimaryIterator it =
      repository_->LowerBoundByNameAndType(start_after_name, start_after_type);
  if (it == repository_->primary_end()) {
    return it;
  }
  // We are supposed to start *after* the indicated element. If we get a
  // lower bound that equals the indicated element, we need to increment
  // the iterator first.
  const model::QualifiedName& found = (it->second).entity.name();
  if (start_after_name.name_space() == found.name_space() &&
      start_after_name.name() == found.name() &&
      start_after_type == (it->second).entity.type()) {
    it++;
  }
  return it;
}

grpc::Status RepositoryService::ListJustRepositories(
    RepositoryRepository::PrimaryIterator it,
    uint32_t list_max,
    model::server::ListRepositoriesResponse* response) {
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
    model::server::ListRepositoriesResponse* response) {
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

grpc::Status RepositoryService::ListRepositories(
    uint32_t list_max,
    const model::QualifiedName& start_after_name,
    model::Repository_Type start_after_type,
    bool include_descriptions,
    model::server::ListRepositoriesResponse* response) {
  if (list_max == 0) {
    // Kind of silly, but if the max desired is 0, just return immediately.
    return grpc::Status::OK;
  }

  RepositoryRepository::PrimaryIterator it = IteratorStart(start_after_name,
                                                           start_after_type);

  if (include_descriptions) {
    return ListRepositoriesAndDescriptions(it, list_max, response);
  }
  
  return ListJustRepositories(it, list_max, response);
}

grpc::Status RepositoryService::RemoveRepository(
    const model::QualifiedName& repository_name,
    model::Repository_Type type,
    bool force) {
  if (!force) {
    grpc::Status check = ValidateRepositoryRemoval(repository_name, type);
    if (!check.ok()) {
      return check;
    }
  }

  return repository_->Remove(repository_name, type);
}

grpc::Status RepositoryService::UpdateRepository(
    const model::QualifiedName& name,
    model::Repository_Type type,
    const model::Repository& update) {
  grpc::Status check = ValidateRepositoryUpdate(name, type, update);
  if (check.error_message() == "ok") {
    // This condition tells us that there is no real update required.
    return grpc::Status::OK;
  }

  if (!check.ok()) {
    return check;
  }

  return repository_->UpdateNoDescription(name, type, update);
}

grpc::Status RepositoryService::UpdateRepositoryWithDescription(
    const model::QualifiedName& repository_name,
    model::Repository_Type repository_type,
    const model::Repository& update,
    const model::Description& updated_description,
    bool clear_description) {
  grpc::Status check = ValidateRepositoryUpdate(repository_name,
                                                repository_type,
                                                update);
  if (!check.ok() && check.error_message() != "ok") {
    return check;
  }

  if (check.error_message() == "ok") {
    return grpc::Status::OK;
  }

  if (clear_description) {
    return repository_->UpdateAndClearDescription(repository_name,
                                                  repository_type,
                                                  update);
  } 

  return repository_->UpdateWithDescription(repository_name,
                                            repository_type,
                                            update,
                                            updated_description);
}

grpc::Status RepositoryService::UpsertRepositoryDescription(
    const model::QualifiedName& repository_name,
    model::Repository_Type repository_type,
    const model::Description& update,
    bool clear_description) {
  if (clear_description) {
    return repository_->ClearDescription(repository_name, repository_type);
  }

  return repository_->UpdateDescriptionOnly(repository_name,
                                            repository_type,
                                            update);
}

grpc::Status RepositoryService::ValidateNewRepository(
    const model::Repository& repository) {
  // TODO: Implement this check. Basically, we want to make sure that
  //       the required Namespace exists. To do this, we should also
  //       be constructed with the NamespaceRepository.
  //       At the moment, this is just a stub.
  return grpc::Status::OK;
}

grpc::Status RepositoryService::ValidateRepositoryRemoval(
    const model::QualifiedName& repository_name,
    model::Repository_Type type) {
  // TODO: Implement this check once we get a real DatasetRepository class.
  //       At the moment, this is just a stub.
  return grpc::Status::OK;
}

grpc::Status RepositoryService::ValidateRepositoryUpdate(
    const model::QualifiedName& name,
    model::Repository_Type type,
    const model::Repository& repository) {
  grpc::Status result = ValidateNewRepository(repository);
  if (!result.ok()) {
    return result;
  }
  // TODO: FINISH THIS.
  return grpc::Status::OK;
}

} // namespace acumio

