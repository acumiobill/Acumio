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

#include "encrypter.h"

namespace acumio {

class DatasetService {
 public:
  DatasetService();
  ~DatasetService();

  grpc::Status CreateDataset(const model::Dataset& dataset,
                             const model::DatasetDescription& description) {
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "not yet available");
  }

  grpc::Status GetDataset(const model::server::GetDatasetRequest* request,
                          model::server::GetDatasetResponse* response) {
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "not yet available");
  }

  grpc::Status RemoveDataset(const model::QualifiedName& name) {
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "not yet available");
  }

  grpc::Status SearchDatasets(
      const model::server::SearchDatasetsRequest* request,
      model::server::SearchDatasetsResponse* response) {
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "not yet available");
  }

  grpc::Status UpdateDataset(const model::QualifiedName& name,
                             const model::Dataset& dataset) {
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "not yet available");
  }

  grpc::Status UpdateDatasetWithDescription(
      const model::server::UpdateDatasetWithDescriptionRequest* request) {
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "not yet available");
  }

  grpc::Status UpsertDatasetDescription(
      const model::server::UpsertDatasetDescriptionRequest* request)  {
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "not yet available");
  }

 private:

  // Underlying Dataset storage.
  // DatasetRepository repository_;
};
} // namespace acumio

#endif // AcumioServer_DatasetService_h
