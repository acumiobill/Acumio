//============================================================================
// Name        : NamespaceService.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Service for Namespace operations.
//============================================================================

#include "NamespaceService.h"

#include <chrono>

namespace acumio {

namespace {
static int64_t NANOS_PER_SECOND = 1000000000;
typedef std::chrono::time_point<std::chrono::system_clock> WallClockTime;
typedef std::chrono::duration<int64_t, std::nano> NanoTime;

void PopulateTimestampNow(google::protobuf::Timestamp* ts) {
  WallClockTime now = std::chrono::system_clock::now();
  int64_t epoch_nanos = std::chrono::duration_cast<NanoTime>(
      now.time_since_epoch()).count();
  int64_t epoch_seconds = epoch_nanos/NANOS_PER_SECOND;
  int32_t remaining_nanos = (int32_t) (epoch_nanos % NANOS_PER_SECOND);
  ts->set_seconds(epoch_seconds);
  ts->set_nanos(remaining_nanos);
}
} // anonymous namespace.

NamespaceService::NamespaceService(
    std::shared_ptr<NamespaceRepository> repository) :
    repository_(repository) {}

NamespaceService::~NamespaceService() {}

grpc::Status NamespaceService::CreateNamespace(
    const model::Namespace& name_space,
    const model::Description& description) {
  model::DescribedNamespace described_name;
  described_name.name_space.CopyFrom(name_space);
  model::Description* new_version =
      described_name.description_history.add_version();
  new_version->CopyFrom(description);
  PopulateTimestampNow(new_version->mutable_edit_time());
  
  return repository_->Add(described_name);
}

grpc::Status NamespaceService::GetJustNamespace(
    const std::string& full_namespace,
    model::server::GetNamespaceResponse* response) {
  return repository_->Get(full_namespace, response->mutable_name_space());
}

grpc::Status NamespaceService::GetNamespace(
    const std::string& full_namespace,
    bool include_description,
    bool include_description_history,
    model::server::GetNamespaceResponse* response) {
  if (!include_description && !include_description_history) {
    return GetJustNamespace(full_namespace, response);
  }

  // TODO: Re-work this so that we don't have to copy objects around.
  //       The copy process will slow things down a bit.
  model::DescribedNamespace result;
  grpc::Status status = repository_->GetDescribedNamespace(full_namespace,
                                                           &result);
  response->mutable_name_space()->CopyFrom(result.name_space);
  if (include_description_history) {
    response->mutable_description_history()->CopyFrom(
        result.description_history);
  }
  if (include_description) {
    int last_version = result.description_history.version_size() - 1;
    if (last_version > 0) {
      response->mutable_description()->CopyFrom(
          result.description_history.version(last_version));
    }
  }

  return grpc::Status::OK;
}

grpc::Status NamespaceService::RemoveNamespace(
    const std::string& namespace_name) {
  return repository_->Remove(namespace_name);
}

grpc::Status NamespaceService::UpdateNamespace(
    const std::string& namespace_name,
    const model::Namespace& update) {
  return repository_->Update(namespace_name, update);
}

grpc::Status NamespaceService::UpdateNamespaceWithDescription(
    const std::string& namespace_name,
    const model::Namespace& update,
    const model::Description updated_description,
    bool clear_description) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "not available yet.");
}

grpc::Status NamespaceService::UpsertNamespaceDescription(
    const std::string& described, const model::Description& update,
    bool clear_description) {
  return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "not available yet.");
}

} // namespace acumio

