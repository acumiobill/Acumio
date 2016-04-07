//============================================================================
// Name        : DatasetService.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Service for Dataset operations. Skeleton at the moment.
//============================================================================

#include "DatasetService.h"

namespace acumio {
DatasetService::DatasetService(
    std::shared_ptr<DatasetRepository> repository,
    std::shared_ptr<NamespaceRepository> namespace_repository) :
    repository_(repository), namespace_repository_(namespace_repository) {}
DatasetService::~DatasetService() {}
} // namespace acumio
