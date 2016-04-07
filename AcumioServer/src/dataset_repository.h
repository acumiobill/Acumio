#ifndef AcumioServer_dataset_repository_h
#define AcumioServer_dataset_repository_h
//============================================================================
// Name        : dataset_repository.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Provides multi-threaded access to an in-memory
//               repository of Datasets.
//============================================================================

#include <grpc++/support/status.h>
#include "dataset.pb.h"
#include "description.pb.h"
#include "multi_described_repository.h"
#include "multi_description_mutations.h"
#include "names.pb.h"
#include "protobuf_iterator.h"


namespace acumio {
class DatasetRepository {
 public:
  typedef mem_repository::MultiDescribedRepository<model::Dataset> _Repository;
  typedef _Repository::PrimaryIterator PrimaryIterator;
  typedef _Repository::SecondaryIterator SecondaryIterator;

  DatasetRepository();
  ~DatasetRepository();

  inline grpc::Status Add(const model::Dataset& dataset,
                          const model::MultiDescription& description) {
    return repository_->Add(dataset, description);
  }

  grpc::Status GetDataset(const model::QualifiedName& name,
                          model::Dataset* elt) const;

  grpc::Status GetDescription(
      const model::QualifiedName& name,
      proto::ConstProtoIterator<std::string>& description_tags_begin,
      proto::ConstProtoIterator<std::string>& description_tags_end,
      proto::ConstProtoIterator<std::string>& history_tags_begin,
      proto::ConstProtoIterator<std::string>& history_tags_end,
      model::MultiDescription* description,
      model::MultiDescriptionHistory* history) const;

  grpc::Status GetDatasetAndDescription(
      const model::QualifiedName& name,
      proto::ConstProtoIterator<std::string>& description_tags_begin,
      proto::ConstProtoIterator<std::string>& description_tags_end,
      proto::ConstProtoIterator<std::string>& history_tags_begin,
      proto::ConstProtoIterator<std::string>& history_tags_end,
      model::Dataset* elt,
      model::MultiDescription* description,
      model::MultiDescriptionHistory* history) const;

  grpc::Status RemoveDataset(const model::QualifiedName& name);

  grpc::Status UpdateDataset(const model::QualifiedName& name,
                             const model::Dataset& dataset);

  grpc::Status UpdateDatasetWithDescription(
      const model::QualifiedName& name,
      const model::Dataset& dataset,
      const MultiMutationInterface* description_update);

  grpc::Status UpdateDescription(
      const model::QualifiedName& name,
      const MultiMutationInterface* description_update);

  PrimaryIterator LowerBoundByFullName(const model::QualifiedName& name) const;

  inline PrimaryIterator primary_begin() const {
    return repository_->primary_begin();
  }

  inline PrimaryIterator primary_end() const {
    return repository_->primary_end();
  }

  SecondaryIterator LowerBoundByNamespace(const std::string& name_space) const;

  inline const SecondaryIterator namespace_iter_begin() const {
    return repository_->secondary_begin(0);
  }

  inline const SecondaryIterator namespace_iter_end() const {
    return repository_->secondary_end(0);
  }

 private:
  std::unique_ptr<_Repository> repository_;
};

} // namespace acumio

#endif // AcumioServer_dataset_repository_h
