#ifndef AcumioServer_multi_mutation_context_h
#define AcumioServer_multi_mutation_context_h
//============================================================================
// Name        : multi_mutation_context.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : A struct-like class for providing needed context for updates
//               to MultiDescriptions. When performing updates to
//               MultiDescriptions, our APIs are such that we get chains of
//               mutations. However, there is some information that will be
//               true for all of the mutations in the chain, and should not
//               be repeated.
//               The MultiMutationContext class captures this information
//               that applies across all of the chains.
//============================================================================

#include <google/protobuf/timestamp.pb.h>
#include "description.pb.h"

namespace acumio {
class MultiMutationContext {
 public:
  // It is expected that the lifetime of the provided arguments for
  // edit_time, editor, and knowledge_source exceed the lifetime of
  // the MultiMutationContext; the MultiMutationContext will hold
  // references to the arguments rather than copy them. (Since
  // Description_SourceCategory is an enum of course, we just copy it).
  // Any "normalization" of the values - to account for empty
  // edit_times or empty editor attributes - are assumed to occur
  // *before* we build the MultiMutationContext.
  MultiMutationContext(const google::protobuf::Timestamp& edit_time,
                       const std::string& editor,
                       model::Description_SourceCategory source_category,
                       const std::string& knowledge_source);
  MultiMutationContext(const MultiMutationContext& other);
  ~MultiMutationContext();

  inline const google::protobuf::Timestamp& edit_time() const {
    return edit_time_;
  }

  inline const std::string& editor() const { return editor_; }
  inline model::Description_SourceCategory source_category() const {
    return source_category_;
  }

  inline const std::string& knowledge_source() const {
    return knowledge_source_;
  }
 
 private:
  const google::protobuf::Timestamp& edit_time_;
  const std::string& editor_;
  const model::Description_SourceCategory source_category_;
  const std::string& knowledge_source_;
};

} // namespace acumio
#endif // AcumioServer_multi_mutation_context_h
