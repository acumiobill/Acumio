//============================================================================
// Name        : DatasetService.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Service for Dataset operations.
//============================================================================

#include "DatasetService.h"
#include "dataset.pb.h"
#include "dataset_repository.h"
#include "description.pb.h"
#include "protobuf_iterator.h"
#include "referential_service.h"

namespace acumio {
DatasetService::DatasetService(
    DatasetRepository* repository,
    ReferentialService* referential_service) :
    repository_(repository), referential_service_(referential_service),
    mutation_factory_() {}
DatasetService::~DatasetService() {}

grpc::Status DatasetService::CreateDataset(
    const model::Dataset& dataset,
    const model::MultiDescription& description) {
  model::Namespace parent;
  grpc::Status check = referential_service_->GetParentNamespace(
      dataset.physical_name(), model::DATASET, &parent);
  if (!check.ok()) {
    return check;
  }
  return repository_->Add(dataset, description);
}

grpc::Status DatasetService::GetDataset(
    const DatasetService::_RepeatedQualifiedName& physical_name,
    const DatasetService::_RepeatedString& description_tags,
    const DatasetService::_RepeatedString& history_tags,
    DatasetService::_RepeatedDataset* dataset,
    DatasetService::_RepeatedMultiDescription* description,
    DatasetService::_RepeatedMultiDescriptionHistory* description_history) {
  int num_requests = physical_name.size();
  if (num_requests == 0) {
    return grpc::Status::OK;
  }

  proto::ConstProtoIterator<std::string> description_begin(
      description_tags.begin());
  proto::ConstProtoIterator<std::string> description_end(
      description_tags.end());
  proto::ConstProtoIterator<std::string> history_begin(
      history_tags.begin());
  proto::ConstProtoIterator<std::string> history_end(
      history_tags.end());
  model::Dataset* current_dataset = nullptr;
  model::MultiDescription* current_description = nullptr;
  model::MultiDescriptionHistory* current_history = nullptr;

  grpc::Status check;
  for (int i = 0; i < num_requests && check.ok(); i++) {
    current_dataset = dataset->Add();
    current_description = description->Add();
    current_history = description_history->Add();
    check = repository_->GetDatasetAndDescription(
        physical_name.Get(i), description_begin, description_end, history_begin,
        history_end, current_dataset, current_description, current_history);
  }

  return check;
}

grpc::Status DatasetService::RemoveDataset(const model::QualifiedName& name) {
  return repository_->RemoveDataset(name);
}

grpc::Status DatasetService::UpdateDataset(const model::QualifiedName& name,
                                           const model::Dataset& dataset) {
  // Validate that if the parent namespace for the dataset is changing,
  // then the new parent namespace exists.
  if (name.name_space() != dataset.physical_name().name_space()) {
    model::Namespace parent;
    grpc::Status check = referential_service_->GetParentNamespace(
        name, model::DATASET, &parent);
    if (!check.ok()) {
      return check;
    }
  }
  return repository_->UpdateDataset(name, dataset);
}

grpc::Status DatasetService::UpdateDatasetWithDescription(
    const model::QualifiedName& name,
    const model::Dataset& dataset,
    const model::MultiDescriptionMutationChain& description_update) {
  // Validate that if the parent namespace for the dataset is changing,
  // then the new parent namespace exists.
  if (name.name_space() != dataset.physical_name().name_space()) {
    model::Namespace parent;
    grpc::Status check = referential_service_->GetParentNamespace(
        name, model::DATASET, &parent);
    if (!check.ok()) {
      return check;
    }
  }
  std::unique_ptr<MultiMutationInterface> updates =
      mutation_factory_.build(description_update);
  return repository_->UpdateDatasetWithDescription(name, dataset,
                                                   updates.get());
}

grpc::Status DatasetService::UpdateDatasetDescription(
    const model::QualifiedName& name,
    const model::MultiDescriptionMutationChain& description_update) {
  std::unique_ptr<MultiMutationInterface> updates =
      mutation_factory_.build(description_update);
  return repository_->UpdateDescription(name, updates.get());
}

} // namespace acumio
