#ifndef AcumioServer_DatasetService_h
#define AcumioServer_DatasetService_h
//============================================================================
// Name        : DatasetService.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Service for Dataset operations. Skeleton at the moment.
//============================================================================

#include <grpc++/grpc++.h>
#include <server.pb.h>
#include <dataset.pb.h>
#include <description.pb.h>

#include "dataset_repository.h"
#include "encrypter.h"
#include "namespace_repository.h"

namespace acumio {

class DatasetService {
 public:
  typedef google::protobuf::RepeatedPtrField<model::QualifiedName>
      _RepeatedQualifiedName;
  typedef google::protobuf::RepeatedPtrField<std::string> _RepeatedString;
  typedef google::protobuf::RepeatedPtrField<model::Dataset> _RepeatedDataset;
  typedef google::protobuf::RepeatedPtrField<model::MultiDescription>
      _RepeatedMultiDescription;
  typedef google::protobuf::RepeatedPtrField<model::MultiDescriptionHistory>
      _RepeatedMultiDescriptionHistory;

  DatasetService(std::shared_ptr<DatasetRepository> repository,
                 std::shared_ptr<NamespaceRepository> namespace_repository);
  ~DatasetService();

  grpc::Status CreateDataset(const model::Dataset& dataset,
                             const model::MultiDescription& description);

  grpc::Status GetDataset(
      const _RepeatedQualifiedName& physical_name,
      const _RepeatedString& description_tags,
      const _RepeatedString& history_tags,
      _RepeatedDataset* dataset,
      _RepeatedMultiDescription* description,
      _RepeatedMultiDescriptionHistory*  description_history);

  grpc::Status RemoveDataset(const model::QualifiedName& name);

  grpc::Status SearchDatasets(
      const model::server::SearchDatasetsRequest* request,
      model::server::SearchDatasetsResponse* response) {
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "not yet available");
  }

  grpc::Status UpdateDataset(const model::QualifiedName& name,
                             const model::Dataset& dataset);

  grpc::Status UpdateDatasetWithDescription(
      const model::QualifiedName& name,
      const model::Dataset& dataset,
      const model::MultiDescriptionMutationChain& description_update);

  grpc::Status UpdateDatasetDescription(
      const model::QualifiedName& name,
      const model::MultiDescriptionMutationChain& description_update);

 private:

  // Underlying Dataset storage.
  std::shared_ptr<DatasetRepository> repository_;
  std::shared_ptr<NamespaceRepository> namespace_repository_;
  MultiMutationFactory mutation_factory_;
};
} // namespace acumio

#endif // AcumioServer_DatasetService_h
