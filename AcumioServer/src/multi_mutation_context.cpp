//============================================================================
// Name        : multi_mutation_conext.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Implementation of MultiMutationContext methods.
//============================================================================

#include "multi_mutation_context.h"
#include <google/protobuf/timestamp.pb.h>
#include "description.pb.h"

namespace acumio {
MultiMutationContext::MultiMutationContext(
    const google::protobuf::Timestamp& edit_time,
    const std::string& editor,
    model::Description_SourceCategory source_category,
    const std::string& knowledge_source) :
    edit_time_(edit_time), editor_(editor), source_category_(source_category),
    knowledge_source_(knowledge_source) {}

MultiMutationContext::MultiMutationContext(const MultiMutationContext& other) :
    edit_time_(other.edit_time_), editor_(other.editor_),
    source_category_(other.source_category_),
    knowledge_source_(other.knowledge_source_) {}


MultiMutationContext::~MultiMutationContext() {}
} // namespace acumio
