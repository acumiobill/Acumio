#ifndef AcumioServer_described_repository_h
#define AcumioServer_described_repository_h
//============================================================================
// Name        : described_repository.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Provides templated multi-threaded access to an in-memory
//               repository for an entity that is paired with a
//               DescriptionHistory.
//============================================================================

#include "comparable.h"
#include "description.pb.h"
#include "mem_repository.h"
#include "time_util.h"

namespace acumio {

namespace model {
template <class Entity>
struct Described {
  Entity entity;
  DescriptionHistory description_history;
};

} // namespace model

namespace mem_repository {
template <class Entity>
class DescribedKeyExtractorInterface :
    public KeyExtractorInterface<acumio::model::Described<Entity>> {
 public:
  DescribedKeyExtractorInterface(
    std::unique_ptr<KeyExtractorInterface<Entity>> delegate) :
      delegate_(std::move(delegate)) {}
  ~DescribedKeyExtractorInterface() {}
  std::unique_ptr<Comparable> GetKey(
      const acumio::model::Described<Entity>& element) const {
    return delegate_->GetKey(element.entity);
  }
  const KeyExtractorInterface<Entity>& delegate() const {
    return *delegate_;
  }

 private:
  std::unique_ptr<KeyExtractorInterface<Entity>> delegate_;
};

template <class Entity>
class DescribedRepository {
 public:
  typedef DescribedKeyExtractorInterface<Entity> Extractor;
  typedef KeyExtractorInterface<acumio::model::Described<Entity>>
      _BaseExtractor;
  typedef KeyExtractorInterface<Entity> Delegate;
  typedef MemRepository<acumio::model::Described<Entity>> _Repository;
  typedef typename _Repository::PrimaryIterator PrimaryIterator;
  typedef typename _Repository::SecondaryIterator SecondaryIterator;

  DescribedRepository(std::unique_ptr<Delegate> main_extractor,
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

  ~DescribedRepository() {}

  inline uint16_t added_index_count() const {
    return repository_->added_index_count();
  }

  inline const Extractor& main_extractor() const {
    return dynamic_cast<const Extractor&>(repository_->main_extractor());
  }

  inline const Extractor& ith_extractor(int i) const {
    return dynamic_cast<const Extractor&>(repository_->ith_extractor(i));
  }

  inline int32_t size() const { return repository_->size(); }

  inline grpc::Status Add(const acumio::model::Described<Entity>& e) {
    return repository_->Add(e);
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

  grpc::Status GetDescription(const std::unique_ptr<Comparable>& key,
                              acumio::model::Description* description) const {
    typename _Repository::StatusEltConstPtrPair getResult =
        repository_->NonMutableGet(key);
    if (!getResult.first.ok()) {
      return getResult.first;
    }

    const acumio::model::DescriptionHistory& history =
        getResult.second->description_history;
    int num_versions = history.version_size();
    if (num_versions == 0) {
      // No prior description, so we just clear the description handed to us.
      description->Clear();
      return grpc::Status::OK;
    }

    description->CopyFrom(history.version(num_versions - 1));
    return grpc::Status::OK;
  }

  grpc::Status GetDescriptionHistory(
      const std::unique_ptr<Comparable>& key,
      acumio::model::DescriptionHistory* history ) const {
    typename _Repository::StatusEltConstPtrPair getResult =
        repository_->NonMutableGet(key);
    if (!getResult.first.ok()) {
      return getResult.first;
    }

    history->CopyFrom(getResult.second->description_history);
    return grpc::Status::OK;
  }

  grpc::Status GetEntityAndDescription(
      const std::unique_ptr<Comparable>& key,
      Entity* entity,
      acumio::model::Description* description) const {
    typename _Repository::StatusEltConstPtrPair getResult =
        repository_->NonMutableGet(key);
    if (!getResult.first.ok()) {
      return getResult.first;
    }

    entity->CopyFrom(getResult.second->entity);
    const acumio::model::DescriptionHistory& history =
        getResult.second->description_history;
    int num_versions = history.version_size();
    if (num_versions == 0) {
      // No prior description, so we just clear the description handed to us.
      description->Clear();
      return grpc::Status::OK;
    }

    description->CopyFrom(history.version(num_versions - 1));
    return grpc::Status::OK;
  }

  grpc::Status GetEntityAndDescriptionHistory(
      const std::unique_ptr<Comparable>& key,
      Entity* entity,
      acumio::model::DescriptionHistory* history) const {
    typename _Repository::StatusEltConstPtrPair getResult =
        repository_->NonMutableGet(key);
    if (!getResult.first.ok()) {
      return getResult.first;
    }

    entity->CopyFrom(getResult.second->entity);
    history->CopyFrom(getResult.second->description_history);
    return grpc::Status::OK;
  }

  grpc::Status AddWithDescription(const Entity& e,
                                  const acumio::model::Description& desc) {
    acumio::model::Described<Entity> elt;
    elt.entity = e;
    acumio::model::Description* new_desc =
       elt.description_history.add_version();
    new_desc->CopyFrom(desc);
    if (!new_desc->has_edit_time() || new_desc->edit_time().seconds() == 0) {
      acumio::time::SetTimestampToNow(new_desc->mutable_edit_time());
    }

    return repository_->Add(elt);
  }

  grpc::Status AddWithNoDescription(const Entity& e) {
    acumio::model::Described<Entity> elt;
    elt.entity = e;
    return Add(elt);
  }

  // The type for the key should match the type returned by the
  // main_extractor used in the interface. In other words, given
  // an Entity e, and the main_extractor then main_extractor.GetKey(e)
  // could feasibly match the parameter key in this method
  inline grpc::Status Remove(const std::unique_ptr<Comparable>& key) {
    return repository_->Remove(key);
  }

  grpc::Status UpdateNoDescription(
      const std::unique_ptr<Comparable>& key, const Entity& new_value) {
    EntityMutator mutator(new_value);
    
    std::unique_ptr<Comparable> updated_key =
        main_extractor().delegate().GetKey(new_value);
    return repository_->ApplyMutation(
        key,
        updated_key,
        dynamic_cast<ElementMutatorInterface<
                     acumio::model::Described<Entity>>*>(&mutator));
  }

  grpc::Status ClearDescription(const std::unique_ptr<Comparable>& key) {
    ClearDescriptionMutator mutator;
    return repository_->ApplyMutation(key, key, &mutator);
  }

  grpc::Status UpdateDescriptionOnly(
      const std::unique_ptr<Comparable>& key,
      const acumio::model::Description& description) {
    DescriptionMutator mutator(description);
    return repository_->ApplyMutation(key, key, &mutator);
  }

  grpc::Status UpdateAndClearDescription(
      const std::unique_ptr<Comparable>& key, const Entity& new_value) {
    UpdaterWithClearDescription mutator(new_value);
    std::unique_ptr<Comparable> updated_key =
        main_extractor().delegate().GetKey(new_value);
    return repository_->ApplyMutation(key, updated_key, &mutator);
  }

  grpc::Status UpdateWithDescription(
      const std::unique_ptr<Comparable>& key,
      const Entity& new_value,
      const acumio::model::Description& description) {
    EntityAndDescriptionMutator mutator(new_value, description);
    std::unique_ptr<Comparable> updated_key =
        main_extractor().delegate().GetKey(new_value);
    return repository_->ApplyMutation(key, updated_key, &mutator);
  }

  inline PrimaryIterator LowerBound(
      const std::unique_ptr<Comparable>& key) const {
    return repository_->LowerBound(key);
  }

  inline const PrimaryIterator primary_begin() const {
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
      public ElementMutatorInterface<acumio::model::Described<Entity>> {
   public:
    EntityMutator(const Entity& new_value) : new_value_(new_value) {}
    ~EntityMutator() {}
    inline grpc::Status Mutate(acumio::model::Described<Entity>* element) {
      element->entity = new_value_;
      return grpc::Status::OK;
    }
   private:
    const Entity& new_value_;
  };

  class DescriptionMutator :
      public ElementMutatorInterface<acumio::model::Described<Entity>> {
   public:
    DescriptionMutator(const acumio::model::Description& new_description) :
        new_description_(new_description) {}
    ~DescriptionMutator() {}
    grpc::Status Mutate(acumio::model::Described<Entity>* element) {
      acumio::model::DescriptionHistory* history =
          &(element->description_history);
      int num_versions = history->version_size();
      if (num_versions != 0) {
        const acumio::model::Description& last_version =
            history->version(num_versions - 1);
        if (last_version.contents() == new_description_.contents()) {
            return grpc::Status::OK;
        }
      }

      acumio::model::Description* update = history->add_version();
      update->CopyFrom(new_description_);
      if (!update->has_edit_time() || update->edit_time().seconds() == 0) {
        acumio::time::SetTimestampToNow(update->mutable_edit_time());
      }
      // TODO: edit user information. Need to wait until we are properly
      // getting a user identity. User information would need to come
      // with the constructor to this mutator, hence a parameter of
      // the various update methods.

      return grpc::Status::OK;
    }
   private:
    const acumio::model::Description& new_description_;
  };

  class ClearDescriptionMutator :
      public ElementMutatorInterface<acumio::model::Described<Entity>> {
   public:
    ClearDescriptionMutator() {}
    ~ClearDescriptionMutator() {}
    grpc::Status Mutate(acumio::model::Described<Entity>* element) {
      acumio::model::DescriptionHistory* history =
          &(element->description_history);
      int num_versions = history->version_size();
      if (num_versions == 0) {
        return grpc::Status::OK;
      }
      const acumio::model::Description& last_version =
          history->version(num_versions - 1);
      if (last_version.contents().empty()) {
        return grpc::Status::OK;
      }

      acumio::model::Description* update = history->add_version();
      acumio::time::SetTimestampToNow(update->mutable_edit_time());
      // TODO: edit user information. Need to wait until we are properly
      // getting a user identity. User information would need to come
      // with the constructor to this mutator, hence a parameter of
      // the various update methods.

      return grpc::Status::OK;
    }
  };

  class UpdaterWithClearDescription :
      public ElementMutatorInterface<acumio::model::Described<Entity>> {
   public:
    UpdaterWithClearDescription(const Entity& new_value) :
        entity_mutator_(new_value) {}
    ~UpdaterWithClearDescription() {}

    grpc::Status Mutate(acumio::model::Described<Entity>* element) {
      entity_mutator_.Mutate(element);
      description_mutator_.Mutate(element);
      return grpc::Status::OK;
    }

   private:
    EntityMutator entity_mutator_;
    ClearDescriptionMutator description_mutator_;
  };

  class EntityAndDescriptionMutator :
      public ElementMutatorInterface<acumio::model::Described<Entity>> {
   public:
    EntityAndDescriptionMutator(const Entity& new_value,
                                const acumio::model::Description& description) :
        entity_mutator_(new_value), description_mutator_(description) {}
    ~EntityAndDescriptionMutator() {}

    grpc::Status Mutate(acumio::model::Described<Entity>* element) {
      entity_mutator_.Mutate(element);
      description_mutator_.Mutate(element);
      return grpc::Status::OK;
    }
    
   private:
    EntityMutator entity_mutator_;
    DescriptionMutator description_mutator_;
  };
  
  std::unique_ptr<MemRepository<acumio::model::Described<Entity>>> repository_;
};

} // namespace mem_repository
} // namespace acumio


#endif // AcumioServer_described_entity_repository_h
