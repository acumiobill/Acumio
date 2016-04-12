//============================================================================
// Name        : multi_description_mutations.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Provides implementations for classes from
//               multi_description_mutations.h as well as some hidden
//               helper classes to make the implementation complete.
//============================================================================

#include "multi_description_mutations.h"

#include <grpc++/support/status.h>
#include <iterator>
#include <sstream>

#include "model_constants.h"

namespace acumio {
namespace {
typedef google::protobuf::Map<std::string, acumio::model::DescriptionHistory>
    _HistoryMap;

void ClearDescription(model::Description* new_version,
                      const google::protobuf::Timestamp& edit_time,
                      const std::string& editor,
                      model::Description_SourceCategory knowledge_category,
                      const std::string& knowledge_source) {
  new_version->mutable_contents()->clear();
  new_version->mutable_edit_time()->CopyFrom(edit_time);
  new_version->set_editor(editor);
  new_version->set_knowledge_source_category(knowledge_category);
  new_version->set_knowledge_source(knowledge_source);
}

void InitializeDescription(model::Description* new_version,
                           const std::string& description,
                           const google::protobuf::Timestamp& edit_time,
                           const std::string& editor,
                           model::Description_SourceCategory knowledge_category,
                           const std::string& knowledge_source) {
  new_version->set_contents(description);
  new_version->mutable_edit_time()->CopyFrom(edit_time);
  new_version->set_editor(editor);
  new_version->set_knowledge_source_category(knowledge_category);
  new_version->set_knowledge_source(knowledge_source);
}

grpc::Status VerifyHistoryEditTime(
    const model::DescriptionHistory& history,
    const google::protobuf::Timestamp& edit_time,
    const std::string& tag) {
  int num_versions = history.version_size();
  if (num_versions > 0) {
    const google::protobuf::Timestamp& found_time =
        history.version(num_versions - 1).edit_time();
    if (edit_time.seconds() < found_time.seconds() ||
        (edit_time.seconds() == found_time.seconds() &&
        edit_time.nanos() < found_time.nanos())) {
      std::stringstream error;
      error << "Unable to edit description with provided edit time of ("
            << edit_time.seconds()
            << ") seconds and ("
            << edit_time.nanos()
            << "nanos. This comes after the edit time of ("
            << found_time.seconds()
            << ") seconds and ("
            << found_time.nanos()
            << "nanos for the description with tag (\""
            << tag
            << "\").";
      return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, error.str());
    }
  }
  return grpc::Status::OK;
}

// forward-declaration of BuildSingleMutation
std::unique_ptr<MultiMutationInterface> BuildSingleMutation(
    const MultiMutationContext& context,
    const model::MultiDescriptionMutation& mutation,
    model::Description_SourceCategory knowledge_source_category,
    const std::string& knowledge_source);


typedef std::iterator<
    std::forward_iterator_tag,
    std::unique_ptr<MultiMutationInterface>> _StdChainIterator;

class ChainIterator : public _StdChainIterator {
 public:
  ChainIterator(const model::MultiDescriptionMutationChain& chain,
                int index) : chain_(chain),
                             context_(chain.edit_time(), chain.editor(),
                                      chain.knowledge_source_category(),
                                      chain.knowledge_source()),
                             index_(index), translated_(nullptr),
                             tmp_(nullptr) {}
  ChainIterator(const ChainIterator& other) : chain_(other.chain_),
      context_(other.context_), index_(other.index_), translated_(nullptr),
      tmp_(nullptr) {}
  ~ChainIterator() {}

  std::unique_ptr<MultiMutationInterface>& operator*() {
    translated_.reset(nullptr);
    const model::MultiDescriptionMutation& mutation = chain_.chain(index_);
    model::Description_SourceCategory category =
        ((mutation.knowledge_source_category() ==
         model::Description_SourceCategory_NOT_SPECIFIED) ?
         chain_.knowledge_source_category() :
         mutation.knowledge_source_category());
    const std::string& know_source = (mutation.knowledge_source().empty() ?
                                      chain_.knowledge_source() :
                                      mutation.knowledge_source());
    translated_ = BuildSingleMutation(context_, mutation, category,
                                      know_source);
    return translated_;
  }

  ChainIterator& operator++() {
    index_++;
    return *this;
  }

  ChainIterator& operator++(int) {
    tmp_.reset(new ChainIterator(*this));
    index_++;
    return *tmp_;
  }

  bool operator==(const ChainIterator& other) const {
    // We assume by context that these are from the same chain_.
    // This is intended as a quick check - just comparing indices. Of course,
    // it is a monumentally bad idea to compare ChainIterators that are
    // iterating over different chain_ elements.
    return index_ == other.index_;
  }

  bool operator!=(const ChainIterator& other) const {
    return index_ != other.index_;
  }

 private:
  const model::MultiDescriptionMutationChain& chain_;
  MultiMutationContext context_;
  int index_;
  std::unique_ptr<MultiMutationInterface> translated_;
  std::unique_ptr<ChainIterator> tmp_;
};

/************************ IMPLEMENTATION CLASSES ************************/

class ClearAllMultiMutation : public MultiMutationInterface {
 public:
  ClearAllMultiMutation(
     const MultiMutationContext& context,
     model::Description_SourceCategory knowledge_source_category,
     const std::string& knowledge_source) :
     context_(context), knowledge_source_category_(knowledge_source_category),
     knowledge_source_(knowledge_source) {}
  ~ClearAllMultiMutation() {}

  grpc::Status Mutate(model::MultiDescriptionHistory* element) const {
    _HistoryMap* history_map = element->mutable_history();
    const google::protobuf::Timestamp& edit_time = context_.edit_time();

    // First iteration through map: we just want to make sure that the update is
    // valid insofar as the timestamps would be non-decreasing.
    for (auto it = history_map->begin(); it != history_map->end(); it++) {
      grpc::Status check =
          VerifyHistoryEditTime(it->second, edit_time, it->first);
      if (!check.ok()) {
        return check;
      }
    }

    // Next iteration through map is when we perform the update.
    for (auto it = history_map->begin(); it != history_map->end(); it++) {
      model::DescriptionHistory& specific_history = it->second;
      model::Description* new_version = specific_history.add_version();
      ClearDescription(new_version, edit_time, context_.editor(),
                       knowledge_source_category_, knowledge_source_);
    }

    return grpc::Status::OK;
  }

 private:
  const MultiMutationContext& context_;
  model::Description_SourceCategory knowledge_source_category_;
  const std::string& knowledge_source_;
};

class ClearTaggedMultiMutation : public MultiMutationInterface {
 public:
  ClearTaggedMultiMutation(
      const MultiMutationContext& context,
      model::Description_SourceCategory knowledge_source_category,
      const std::string& knowledge_source,
      const std::string& tag) :
      context_(context), knowledge_source_category_(knowledge_source_category),
      knowledge_source_(knowledge_source), tag_(tag) {}
  ~ClearTaggedMultiMutation() {}
  // Finds the DescriptionHistory in the MultiDescriptionHistory by looking
  // up by tag. With this DescriptionHistory, we create a new version, and
  // with this latest version, the description contents will be an empty
  // string.
  // Generates a NOT_FOUND error if there is no tag matching the tag provided
  // in the constructor.
  grpc::Status Mutate(model::MultiDescriptionHistory* element) const {
    _HistoryMap* history_map = element->mutable_history();
    const google::protobuf::Timestamp& edit_time = context_.edit_time();

    if (history_map->count(tag_) == 0) {
      std::stringstream error;
      error << "Unable to clear attribute (\"" << tag_
            << "\"). The attribute was not found.";
      return grpc::Status(grpc::StatusCode::NOT_FOUND, error.str());
    }
    grpc::Status check =
        VerifyHistoryEditTime(history_map->at(tag_), edit_time, tag_);
    if (!check.ok()) {
      return check;
    }

    model::DescriptionHistory& tagged_history = history_map->at(tag_);
    model::Description* new_version = tagged_history.add_version();
    ClearDescription(new_version, edit_time, context_.editor(),
                     knowledge_source_category_, knowledge_source_);
    return grpc::Status::OK;
  }

 private:
  const MultiMutationContext& context_;
  model::Description_SourceCategory knowledge_source_category_;
  const std::string& knowledge_source_;
  const std::string& tag_;
};

class RemoveAllMultiMutation : public MultiMutationInterface {
 public:
  RemoveAllMultiMutation() {}
  ~RemoveAllMultiMutation() {}
  // Effectively, removes all DescriptionHistory and the
  // MultiDescriptionHistory is set to just have an empty map.
  // Context and version information is not useful because all
  // history is erased. (TODO: Consider a means of truly going
  // back in time for a given MultiDescriptionHistory. The model
  // for the MultiDescriptionHistory is one where it is basically
  // an independent series of DescriptionHistories looked up by tag).
  grpc::Status Mutate(model::MultiDescriptionHistory* element) const {
    element->mutable_history()->clear();
    return grpc::Status::OK;
  }
};

class RemoveTaggedMultiMutation : public MultiMutationInterface {
 public:
  RemoveTaggedMultiMutation(const std::string& tag) : tag_(tag) {}
  ~RemoveTaggedMultiMutation() {}
  // The given MultiDescriptionHistory is searched for a tag matching the
  // "tag" attribute provided in the constructor, and if found, we remove
  // it entirely. Since we are not versioning the MultiDescriptionHistory
  // as a whole (only versioning its elements, which are treated separately),
  // there is no context information applied.
  // This will generate a NOT_FOUND error if there is no tag matching the
  // tag parameter used in the constructor.
  grpc::Status Mutate(model::MultiDescriptionHistory* element) const {
    if (element->history().count(tag_) == 0) {
      std::stringstream error;
      error << "Unable to remove attribute (\"" << tag_
            << "\"). The attribute was not found.";
      return grpc::Status(grpc::StatusCode::NOT_FOUND, error.str());
    }
    element->mutable_history()->erase(tag_);
    return grpc::Status::OK;
  }

 private:
  const std::string& tag_;
};

class RenameMultiMutation : public MultiMutationInterface {
 public:
  RenameMultiMutation(const std::string& from_name,
                      const std::string& to_name) :
      from_name_(from_name), to_name_(to_name) {}
  ~RenameMultiMutation() {}
  // Finds the DescriptionHistory tagged by "from_name" (provided in the
  // constructor) and renames the tag to "to_name". Since no new version
  // is created by this process, we do not need the usual context information.
  // The prior history for the given tag will now apply to the new tag.
  //
  // This will generate a NOT_FOUND error if there is no tag matching
  // from_name, and -- assuming that there is a from_name -- it will
  // generate an ALREADY_EXISTS error if there is already a tag matching
  // to_name.
  grpc::Status Mutate(model::MultiDescriptionHistory* element) const {
    if (element->history().count(to_name_) > 0) {
      std::stringstream error;
      error << "Unable to rename attribute (\"" << from_name_
            << "\") to (\"" << to_name_
            << "\") since there is already an attribute with the name (\""
            << to_name_ << "\").";
      return grpc::Status(grpc::StatusCode::NOT_FOUND, error.str());
    }
    if (element->history().count(from_name_) == 0) {
      std::stringstream error;
      error << "Unable to rename attribute (\"" << from_name_
            << "\") to (\"" << to_name_
            << "\") since there is no attribute with the name (\""
            << from_name_ << "\").";
      return grpc::Status(grpc::StatusCode::ALREADY_EXISTS, error.str());
    }

    _HistoryMap* history_map = element->mutable_history();
    history_map->at(to_name_).CopyFrom(history_map->at(from_name_));
    history_map->erase(from_name_);
    return grpc::Status::OK;
  }

 private:
  const std::string& from_name_;
  const std::string& to_name_;
};

class UpsertMultiMutation : public MultiMutationInterface {
 public:
  UpsertMultiMutation(
      const MultiMutationContext& context,
      model::Description_SourceCategory knowledge_source_category,
      const std::string& knowledge_source,
      const std::string& tag,
      const std::string& description) :
      context_(context), knowledge_source_category_(knowledge_source_category),
      knowledge_source_(knowledge_source), tag_(tag),
      description_(description) {}
  ~UpsertMultiMutation() {}
  // Finds the DescriptionHistory tagged by "tag" (provided in the constructor)
  // and updates the history by making a new version with the version's content
  // attribute set to description (from the constructor) and the other version
  // attributes set according to the other values.
  // This will instead however, generate an error if there is a version with
  // a timestamp that is later than the timestamp provided by the context.
  grpc::Status Mutate(model::MultiDescriptionHistory* element) const {
    _HistoryMap* history_map = element->mutable_history();
    const google::protobuf::Timestamp& edit_time = context_.edit_time();
    model::Description* new_version = nullptr;
    if (history_map->count(tag_) == 0) {
      // In this case, we are adding a new DescriptionHistory with a single
      // version. We accept any edit time that has actually been initialized.
      if (edit_time.seconds() == 0 && edit_time.nanos() == 0) {
        std::stringstream error;
        error << "Unable to create description with tag (\"" << tag_
              << "\") since the edit time was not initialized.";
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, error.str());
      }

      model::DescriptionHistory new_description;
      new_version = new_description.add_version();
      InitializeDescription(new_version, description_, edit_time,
                            context_.editor(), knowledge_source_category_,
                            knowledge_source_);
      (*history_map)[tag_] = new_description;
      return grpc::Status::OK;
    }

    model::DescriptionHistory& specific_history = history_map->at(tag_);
    grpc::Status check = VerifyHistoryEditTime(specific_history, edit_time,
                                               tag_);
    if (!check.ok()) {
      return check;
    }

    new_version = specific_history.add_version();
    InitializeDescription(new_version, description_, edit_time,
                          context_.editor(), knowledge_source_category_,
                          knowledge_source_);

    return grpc::Status::OK;
  }

 private:
  const MultiMutationContext& context_;
  model::Description_SourceCategory knowledge_source_category_;
  const std::string& knowledge_source_;
  const std::string& tag_;
  const std::string& description_;
};

// Creates a single element of a MultiDescriptionMutationChain.
std::unique_ptr<MultiMutationInterface> BuildSingleMutation(
    const MultiMutationContext& context,
    const model::MultiDescriptionMutation& mutation,
    model::Description_SourceCategory knowledge_source_category,
    const std::string& knowledge_source) {
  std::unique_ptr<MultiMutationInterface> ret_val(nullptr);
  switch(mutation.operation_case()) {
    case model::MultiDescriptionMutation::OperationCase::kClear:
      if (mutation.clear() == acumio::model::WILDCARD) {
        ret_val.reset(
            new ClearAllMultiMutation(context, knowledge_source_category,
                                      knowledge_source));
      } else {
        ret_val.reset(
            new ClearTaggedMultiMutation(context, knowledge_source_category,
                                         knowledge_source, mutation.clear()));
      }
      break;
    case model::MultiDescriptionMutation::OperationCase::kRemove:
      if (mutation.remove() == acumio::model::WILDCARD) {
        ret_val.reset(new RemoveAllMultiMutation());
      } else {
        ret_val.reset(new RemoveTaggedMultiMutation(mutation.remove()));
      }
      break;
    case model::MultiDescriptionMutation::OperationCase::kRename:
      ret_val.reset(
          new RenameMultiMutation(mutation.rename().from_name(),
                                  mutation.rename().to_name()));
      break;
    case model::MultiDescriptionMutation::OperationCase::kUpsert:
      ret_val.reset(
          new UpsertMultiMutation(context, knowledge_source_category,
                                  knowledge_source, mutation.upsert().tag(),
                                  mutation.upsert().description()));
      break;
    default: // Intentional no-op. We just return the nullptr result.
      break;
  }
  return ret_val;
}

class ChainedMultiMutation : public MultiMutationInterface {
 public:
  ChainedMultiMutation(ChainIterator begin, ChainIterator end) :
      begin_(begin), end_(end) {}
  ~ChainedMultiMutation() {}
  grpc::Status Mutate(model::MultiDescriptionHistory* element) const {
    grpc::Status check;
    for (ChainIterator it = begin_; it != end_; it++) {
      std::unique_ptr<MultiMutationInterface> mutation = std::move(*it);
      check = mutation->Mutate(element);
      if (!check.ok()) {
        return check;
      }
    }
    return grpc::Status::OK;
  }

 private:
  ChainIterator begin_;
  ChainIterator end_;
};

} // end anonymous namespace

MultiMutationInterface::MultiMutationInterface() {}
MultiMutationInterface::~MultiMutationInterface() {}

std::unique_ptr<MultiMutationInterface> MultiMutationFactory::build(
    const model::MultiDescriptionMutationChain& chain) const {
  ChainIterator begin(chain, 0);
  ChainIterator end(chain, chain.chain_size());
  std::unique_ptr<MultiMutationInterface> ret_val(
      new ChainedMultiMutation(begin, end));
  return ret_val;
}
} // namespace acumio
