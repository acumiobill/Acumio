#ifndef AcumioServer_multi_described_repository_h
#define AcumioServer_multi_described_repository_h
//============================================================================
// Name        : multi_described_repository.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Provides templated multi-threaded access to an in-memory
//               repository for an entity that is paired with a
//               MultiDescriptionHistory.
//============================================================================

#include "comparable.h"
#include "description.pb.h"
#include "mem_repository.h"
#include "model_constants.h"
#include "multi_description_mutations.h"
#include "protobuf_iterator.h"
#include "time_util.h"

namespace acumio {

namespace model {
template <class Entity>
struct MultiDescribed {
  Entity entity;
  MultiDescriptionHistory history;
};
} // namespace model

namespace mem_repository {

namespace {
grpc::Status PopulateMultiDescriptionAllTags(
    const acumio::model::MultiDescriptionHistory& history,
    acumio::model::MultiDescription* description) {
  auto description_map = description->mutable_description();
  auto it_end = history.history().end();
  for (auto it = history.history().begin(); it != it_end; it++) {
    const acumio::model::DescriptionHistory& specific_history = it->second;
    int num_versions = specific_history.version_size();
    if (num_versions > 0) {
      (*description_map)[it->first] =
          specific_history.version(num_versions - 1);
    }
  }

  return grpc::Status::OK;
}

grpc::Status PopulateMultiDescription(
    const acumio::model::MultiDescriptionHistory& history,
    acumio::proto::ConstProtoIterator<std::string> tags_begin,
    acumio::proto::ConstProtoIterator<std::string> tags_end,
    acumio::model::MultiDescription* description) {
  if (tags_begin == tags_end) {
    return grpc::Status::OK;
  }

  if (*tags_begin == acumio::model::WILDCARD) {
    return PopulateMultiDescriptionAllTags(history, description);
  }

  auto history_map = history.history();
  auto description_map = description->mutable_description();
  for (auto it = tags_begin; it != tags_end; it++) {
    if (history_map.count(*it) > 0) {
      const model::DescriptionHistory& specific_history = history_map[*it];
      int num_versions = specific_history.version_size();
      if  (num_versions > 0) {
        (*description_map)[*it] = specific_history.version(num_versions - 1);
      }
    }
  }
  return grpc::Status::OK;
}

grpc::Status PopulateMultiDescriptionHistory(
    const acumio::model::MultiDescriptionHistory& source,
    acumio::proto::ConstProtoIterator<std::string> tags_begin,
    acumio::proto::ConstProtoIterator<std::string> tags_end,
    acumio::model::MultiDescriptionHistory* target) {
  if (tags_begin == tags_end) {
    return grpc::Status::OK;
  }

  if (*tags_begin == acumio::model::WILDCARD) {
    target->CopyFrom(source);
    return grpc::Status::OK;
  }

  auto source_history_map = source.history();
  auto target_history_map = target->mutable_history();
  for (auto it = tags_begin; it != tags_end; it++) {
    const std::string& tag = *it;
    if (source_history_map.count(tag) >  0) {
      const acumio::model::DescriptionHistory& specific_source =
          source_history_map[tag];
      (*target_history_map)[tag] = specific_source;
    }
  }
  return grpc::Status::OK;
}
} // anonymous namespace

template <class Entity>
class MultiDescribedKeyExtractorInterface :
    public KeyExtractorInterface<acumio::model::MultiDescribed<Entity>> {
 public:
  MultiDescribedKeyExtractorInterface(
    std::unique_ptr<KeyExtractorInterface<Entity>> delegate) :
      delegate_(std::move(delegate)) {}
  ~MultiDescribedKeyExtractorInterface() {}
  std::unique_ptr<Comparable> GetKey(
      const acumio::model::MultiDescribed<Entity>& element) const {
    return delegate_->GetKey(element.entity);
  }
  const KeyExtractorInterface<Entity>& delegate() const {
    return *delegate_;
  }

 private:
  std::unique_ptr<KeyExtractorInterface<Entity>> delegate_;
};

template <class Entity>
class MultiDescribedRepository {
 public:
  typedef MultiDescribedKeyExtractorInterface<Entity> Extractor;
  typedef KeyExtractorInterface<acumio::model::MultiDescribed<Entity>>
      _BaseExtractor;
  typedef KeyExtractorInterface<Entity> Delegate;
  typedef MemRepository<acumio::model::MultiDescribed<Entity>> _Repository;
  typedef typename _Repository::PrimaryIterator PrimaryIterator;
  typedef typename _Repository::SecondaryIterator SecondaryIterator;
  typedef google::protobuf::Map<std::string, acumio::model::Description>
      DescriptionMap;
  typedef google::protobuf::Map<std::string, acumio::model::DescriptionHistory>
      DescriptionHistoryMap;

  MultiDescribedRepository(std::unique_ptr<Delegate> main_extractor,
                           std::vector<std::unique_ptr<Delegate>>* extractors) {
    std::unique_ptr<Extractor> described_main_extractor(
        new Extractor(std::move(main_extractor)));
    std::vector<std::unique_ptr<_BaseExtractor>> described_extractors;
    for (uint32_t i = 0; i < extractors->size(); i++) {
      std::unique_ptr<_BaseExtractor> current(
          dynamic_cast<_BaseExtractor*>(
              new Extractor(std::move(extractors->at(i)))));
      described_extractors.push_back(std::move(current));
    }
    repository_.reset(new _Repository(std::move(described_main_extractor),
                                      &described_extractors));
  }

  ~MultiDescribedRepository() {}

  inline uint16_t added_index_count() const {
    return repository_->added_index_count();
  }

  inline const Extractor& main_extractor() const {
    return dynamic_cast<const Extractor&>(repository_->main_extractor());
  }

  inline const Extractor& ith_extractor(int i) const {
    return dynamic_cast<const Extractor&>(repository_->ith_extractor(i));
  }

  inline grpc::Status Add(const acumio::model::MultiDescribed<Entity>& e) {
    return repository_->Add(e);
  }

  grpc::Status Add(const Entity& e,
                   const model::MultiDescription& description) {
    acumio::model::MultiDescribed<Entity> elt;
    elt.entity = e;
    // type for history_map is pointer-to-map: string --> DescriptionHistory
    auto history_map = elt.history.mutable_history();

    auto end_iter = description.description().end();
    for (auto it = description.description().begin(); it != end_iter; it++) {
      model::DescriptionHistory specific_history;
      model::Description* new_desc = specific_history.add_version();
      new_desc->CopyFrom(it->second);
      if (!new_desc->has_edit_time() || new_desc->edit_time().seconds() == 0) {
        acumio::time::SetTimestampToNow(new_desc->mutable_edit_time());
      }
      (*history_map)[it->first] = specific_history;
    }

    return Add(elt); 
  }

  grpc::Status GetEntity(const std::unique_ptr<Comparable>& key,
                         Entity* entity) const {
    typename _Repository::StatusEltConstPtrPair getResult =
        repository_->NonMutableGet(key);
    if (!getResult.first.ok()) {
      return getResult.first;
    }

    entity->CopyFrom(getResult.second->entity);
    return grpc::Status::OK;
  }

  grpc::Status GetDescription(
      const std::unique_ptr<Comparable>& key,
      acumio::proto::ConstProtoIterator<std::string>& description_tags_begin,
      acumio::proto::ConstProtoIterator<std::string>& description_tags_end,
      acumio::proto::ConstProtoIterator<std::string>& history_tags_begin,
      acumio::proto::ConstProtoIterator<std::string>& history_tags_end,
      acumio::model::MultiDescription* description,
      acumio::model::MultiDescriptionHistory* history) const {
    typename _Repository::StatusEltConstPtrPair getResult =
        repository_->NonMutableGet(key);
    if (!getResult.first.ok()) {
      return getResult.first;
    }

    const acumio::model::MultiDescriptionHistory& source_history =
        getResult.second->history;

    grpc::Status check = PopulateMultiDescription(
        source_history, description_tags_begin, description_tags_end,
        description);
    if (!check.ok()) {
      return check;
    }

    return PopulateMultiDescriptionHistory(
        source_history, history_tags_begin, history_tags_end, history);
  }

  grpc::Status GetEntityAndDescription(
      const std::unique_ptr<Comparable>& key,
      acumio::proto::ConstProtoIterator<std::string>& description_tags_begin,
      acumio::proto::ConstProtoIterator<std::string>& description_tags_end,
      acumio::proto::ConstProtoIterator<std::string>& history_tags_begin,
      acumio::proto::ConstProtoIterator<std::string>& history_tags_end,
      Entity* entity,
      acumio::model::MultiDescription* description,
      acumio::model::MultiDescriptionHistory* history) const {
    typename _Repository::StatusEltConstPtrPair getResult =
        repository_->NonMutableGet(key);
    if (!getResult.first.ok()) {
      return getResult.first;
    }

    entity->CopyFrom(getResult.second->entity);
    const acumio::model::MultiDescriptionHistory& source_history =
        getResult.second->history;

    grpc::Status check = PopulateMultiDescription(
        source_history, description_tags_begin, description_tags_end,
        description);
    if (!check.ok()) {
      return check;
    }

    return PopulateMultiDescriptionHistory(
        source_history, history_tags_begin, history_tags_end, history);
  }

  // The type for the key should match the type returned by the
  // main_extractor used in the interface. In other words, given
  // an Entity e, and the main_extractor then main_extractor.GetKey(e)
  // could feasibly match the parameter key in this method
  inline grpc::Status Remove(const std::unique_ptr<Comparable>& key) {
    return repository_->Remove(key);
  }

  grpc::Status Update(
      const std::unique_ptr<Comparable>& key, const Entity& new_value) {
    EntityMutator mutator(new_value);

    std::unique_ptr<Comparable> updated_key =
        main_extractor().delegate().GetKey(new_value);
    return repository_->ApplyMutation(
        key,
        updated_key,
        dynamic_cast<ElementMutatorInterface<
                     acumio::model::MultiDescribed<Entity>>*>(&mutator));
  }

  grpc::Status UpdateWithDescription(
     const std::unique_ptr<Comparable>& key,
     const Entity& new_value,
     const MultiMutationInterface* description_update) {
    EntityAndDescriptionMutator mutator(new_value, description_update);
 
    std::unique_ptr<Comparable> updated_key =
        main_extractor().delegate().GetKey(new_value);
    return repository_->ApplyMutation(
        key,
        updated_key,
        dynamic_cast<ElementMutatorInterface<
                     acumio::model::MultiDescribed<Entity>>*>(&mutator));
  }

  grpc::Status UpdateDescription(
      const std::unique_ptr<Comparable>& key,
      const MultiMutationInterface* description_update) {
    DescriptionOnlyMutator mutator(description_update);

    // The ApplyMutation method takes a pair of *references* to unique_ptrs,
    // not unique_ptrs. That's why the following operation is safe. The
    // storage is still owned by the caller. Note of course that if it
    // took just a unique_ptr then the operation below would fail because
    // the memory would have to be removed from one of the calling arguments.
    return repository_->ApplyMutation(
        key,
        key,
        dynamic_cast<ElementMutatorInterface<
                     acumio::model::MultiDescribed<Entity>>*>(&mutator));
  }

  inline PrimaryIterator LowerBound(
      const std::unique_ptr<Comparable>& key) const {
    return repository_->LowerBound(key);
  }

  inline PrimaryIterator primary_begin() const {
    return repository_->primary_begin();
  }

  inline const PrimaryIterator primary_end() const {
    return repository_->primary_end();
  }

  inline SecondaryIterator LowerBoundByIndex(
      const std::unique_ptr<Comparable>& key, int index_number) const {
    return repository_->LowerBoundByIndex(key, index_number);
  }

  inline const SecondaryIterator secondary_begin(int index_number) const {
    return repository_->secondary_begin(index_number);
  }

  inline const SecondaryIterator secondary_end(int index_number) const {
    return repository_->secondary_end(index_number);
  }

 private:
  class EntityMutator :
      public ElementMutatorInterface<acumio::model::MultiDescribed<Entity>> {
   public:
    EntityMutator(const Entity& new_value) : new_value_(new_value) {}
    ~EntityMutator() {}
    inline grpc::Status Mutate(acumio::model::MultiDescribed<Entity>* element) {
      element->entity = new_value_;
      return grpc::Status::OK;
    }
   private:
    const Entity& new_value_;
  }; // end inner class EntityMutator

  class DescriptionOnlyMutator :
      public ElementMutatorInterface<acumio::model::MultiDescribed<Entity>> {
   public:
    DescriptionOnlyMutator(const MultiMutationInterface* description_update) :
        description_update_(description_update) {}
    ~DescriptionOnlyMutator() {}
    inline grpc::Status Mutate(acumio::model::MultiDescribed<Entity>* element) {
      return description_update_->Mutate(&(element->history));
    }

   private:
    const MultiMutationInterface* description_update_;
  }; // end inner class DescriptionOnlyMutator

  class EntityAndDescriptionMutator :
      public ElementMutatorInterface<acumio::model::MultiDescribed<Entity>> {
   public:
    EntityAndDescriptionMutator(
        const Entity& new_value,
        const MultiMutationInterface* description_update) :
        new_value_(new_value), description_update_(description_update) {}
    ~EntityAndDescriptionMutator() {}
    inline grpc::Status Mutate(acumio::model::MultiDescribed<Entity>* element) {
      element->entity = new_value_;
      return description_update_->Mutate(&(element->history));
    }

   private:
    const Entity& new_value_;
    const MultiMutationInterface* description_update_;
  }; // end inner class EntityAndDescriptionMutator

  std::unique_ptr<_Repository> repository_;
};

} // namespace mem_repository
} // namespace acumio


#endif // AcumioServer_multi_described_repository_h
