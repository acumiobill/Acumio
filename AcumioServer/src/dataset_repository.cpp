//============================================================================
// Name        : dataset_repository.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Provides multi-threaded access to an in-memory repository
//               of databases.
//============================================================================

#include "dataset_repository.h"

namespace acumio {

namespace {
typedef mem_repository::KeyExtractorInterface<model::Dataset> _DatasetExtractor;

class DatasetKeyExtractor : public _DatasetExtractor {
 public:
  DatasetKeyExtractor() : _DatasetExtractor() {}
  ~DatasetKeyExtractor() {}
  inline std::unique_ptr<Comparable> GetKey(
      const model::Dataset& dataset) const {
    std::unique_ptr<Comparable> key(
        new StringPairComparable(dataset.physical_name().name_space(),
                                 dataset.physical_name().name()));
    return key;
  }
};

class DatasetNamespaceExtractor : public _DatasetExtractor {
 public:
  DatasetNamespaceExtractor() : _DatasetExtractor() {}
  ~DatasetNamespaceExtractor() {}
  inline std::unique_ptr<Comparable> GetKey(
      const model::Dataset& dataset) const {
    std::unique_ptr<Comparable> key(
        new StringComparable(dataset.physical_name().name_space()));
    return key;
  }
};

} // anonymous namespace

DatasetRepository::DatasetRepository() : repository_(nullptr) {
  std::unique_ptr<_DatasetExtractor> main_extractor(
      new DatasetKeyExtractor());
  std::unique_ptr<_DatasetExtractor> namespace_extractor(
      new DatasetNamespaceExtractor());

  std::vector<std::unique_ptr<_DatasetExtractor>> additional_extractors;
  additional_extractors.push_back(std::move(namespace_extractor));
  repository_.reset(new _Repository(std::move(main_extractor),
                                    &additional_extractors));
}

DatasetRepository::~DatasetRepository() {}

grpc::Status DatasetRepository::GetDataset(const model::QualifiedName& name,
                                           model::Dataset* elt) const {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(name.name_space(), name.name()));

  // TODO: Translate NOT_FOUND results to present better error message.
  // See similar comment in NamespaceRepository::GetDescription.
  return repository_->GetEntity(key, elt);
}

grpc::Status DatasetRepository::GetDescription(
    const model::QualifiedName& name,
    proto::ConstProtoIterator<std::string>& description_tags_begin,
    proto::ConstProtoIterator<std::string>& description_tags_end,
    proto::ConstProtoIterator<std::string>& history_tags_begin,
    proto::ConstProtoIterator<std::string>& history_tags_end,
    model::MultiDescription* description,
    model::MultiDescriptionHistory* history) const {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(name.name_space(), name.name()));
  return repository_->GetDescription(
      key, description_tags_begin, description_tags_end, history_tags_begin,
      history_tags_end, description, history);
}

grpc::Status DatasetRepository::GetDatasetAndDescription(
    const model::QualifiedName& name,
    proto::ConstProtoIterator<std::string>& description_tags_begin,
    proto::ConstProtoIterator<std::string>& description_tags_end,
    proto::ConstProtoIterator<std::string>& history_tags_begin,
    proto::ConstProtoIterator<std::string>& history_tags_end,
    model::Dataset* elt,
    model::MultiDescription* description,
    model::MultiDescriptionHistory* history) const {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(name.name_space(), name.name()));

  return repository_->GetEntityAndDescription(
      key, description_tags_begin, description_tags_end, history_tags_begin,
      history_tags_end, elt, description, history);
}

grpc::Status DatasetRepository::RemoveDataset(
    const model::QualifiedName& name) {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(name.name_space(), name.name()));
  return repository_->Remove(key);
}

grpc::Status DatasetRepository::UpdateDataset(const model::QualifiedName& name,
                                              const model::Dataset& dataset) {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(name.name_space(), name.name()));
  return repository_->Update(key, dataset);
}

grpc::Status DatasetRepository::UpdateDatasetWithDescription(
    const model::QualifiedName& name,
    const model::Dataset& dataset,
    const MultiMutationInterface* description_update) {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(name.name_space(), name.name()));
  return repository_->UpdateWithDescription(key, dataset, description_update);
}

grpc::Status DatasetRepository::UpdateDescription(
    const model::QualifiedName& name,
    const MultiMutationInterface* description_update) {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(name.name_space(), name.name()));
  return repository_->UpdateDescription(key, description_update);
}


DatasetRepository::PrimaryIterator DatasetRepository::LowerBoundByFullName(
    const model::QualifiedName& name) const {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(name.name_space(), name.name()));
  return repository_->LowerBound(key);
}

DatasetRepository::SecondaryIterator DatasetRepository::LowerBoundByNamespace(
    const std::string& name_space) const {
  std::unique_ptr<Comparable> key(new StringComparable(name_space));
  return repository_->LowerBoundByIndex(key, 0);
}

} // namespace acumio
