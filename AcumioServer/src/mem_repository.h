#ifndef AcumioServer_mem_repository_h
#define AcumioServer_mem_repository_h
//============================================================================
// Name        : mem_repository.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Provides templated multi-threaded access to an in-memory
//               repository. (Not yet threadsafe).
//============================================================================

#include <iostream> // Remove me. Needed for std::cout.
#include <map>
#include <sstream>
#include <stack>
#include <stdint.h>
#include <vector>
#include <grpc++/support/status.h>
#include "comparable.h"
#include "pointer_less.h"

namespace acumio {

namespace mem_repository {

template <class EltType>
class KeyExtractorInterface {
 public:
  KeyExtractorInterface() {}
  virtual ~KeyExtractorInterface() {}
  virtual std::unique_ptr<Comparable> GetKey(const EltType& element) const = 0;
};

template <class EltType>
class ElementMutatorInterface {
 public:
  ElementMutatorInterface() {}
  virtual ~ElementMutatorInterface() {}
  virtual grpc::Status Mutate(EltType* element) = 0;
};

template <class EltType>
class ReplacementMutator : public ElementMutatorInterface<EltType> {
 public:
  ReplacementMutator(const EltType& replacement) :
      ElementMutatorInterface<EltType>(), replacement_(replacement) {}
  ~ReplacementMutator() {}
  grpc::Status Mutate(EltType* element) {
    *element = replacement_;
    return grpc::Status::OK;
  }

 private:
  EltType replacement_;
};

template <class EltType>
class MemRepository {
 public:
  typedef KeyExtractorInterface<EltType> Extractor;
  // A few considerations:
  // First:
  // If we have a well-written trie (patricia-trie) structure,
  // it will be more efficient. Since the std::map uses a red-black tree
  // implementation, with a thousand elements in the map, 10 comparisons
  // are required to find the leaves of the tree. If each comparison is
  // a string-compare, this can be a bit costly. The cost would be
  // O(log(n) * m) where n is the number of elements and m is the length
  // of the string. By contrast, a trie structure would require only
  // O(m) compares.
  // Second:
  // We need thread-safe access to this content, particularly in the case
  // of a multiply-indexed dataset. However, locking the entire structure
  // is cost-prohibitive. An alternative is to partition the maps into
  // chunks such that, if a map contains n elements, each chunk will
  // contain O(sqrt(n)) elements, and then there is a map that selects
  // which of the chunks you need to go in. The super-map is of course,
  // of size O(sqrt(n)). Now, when a mutation occurs, you only need to
  // lock the specific chunk(s) involved rather than the entire
  // structure. A caveat is that occasionally, the chunk boundaries need
  // to be adjusted. This can be detected and performed by a background
  // process.
  // Third:
  // Having a direct hash-map access might also be helpful based on
  // primary-key lookup. However, if we have a well-written
  // patricia-trie, this will not really be required.
  // Fourth:
  // Some indexes, such as indexes based on keyword search, will be
  // expensive to perform up-front. For those indices, it is better
  // to have a background process that does the indexing operation.
  typedef std::map<std::unique_ptr<Comparable>, int32_t,
                   acumio::functional::pointer_less<Comparable>> RepositoryMap;

  typedef std::multimap<std::unique_ptr<Comparable>, int32_t,
                        acumio::functional::pointer_less<Comparable>>
      RepositoryMultiMap;

  typedef std::pair<const std::unique_ptr<Comparable>&, const EltType>
      IteratorElement;
  typedef std::pair<grpc::Status, EltType*> StatusEltPtrPair;
  typedef std::pair<grpc::Status, const EltType*> StatusEltConstPtrPair;

  template <typename IterType>
  class Iterator :
      public std::iterator<std::bidirectional_iterator_tag, IteratorElement> {
   public:
    Iterator<IterType>() {}
    Iterator<IterType>(IterType wrapped_iterator,
                       const std::vector<EltType>* elements) :
        wrapped_iterator_(wrapped_iterator), elements_(elements),
        saved_elt_(nullptr) {}
    Iterator<IterType>(const Iterator<IterType>& copy) :
        wrapped_iterator_(copy.wrapped_iterator_), elements_(copy.elements_) {}
    ~Iterator<IterType>() {}
    // pre-increment/decrement.
    inline Iterator<IterType>& operator++() {
      wrapped_iterator_++;
      return *this;
    }
    inline Iterator<IterType>& operator--() {
      wrapped_iterator_--;
      return *this;
    }
    // post-increment/decrement.
    inline Iterator<IterType> operator++(int) {
      Iterator<IterType> tmp(*this);
      wrapped_iterator_++;
      return tmp;
    }
    inline Iterator<IterType> operator--(int) {
      Iterator<IterType> tmp(*this);
      wrapped_iterator_--;
      return tmp;
    }
    inline bool operator==(const Iterator<IterType>& other) const {
      return wrapped_iterator_ == other.wrapped_iterator_ &&
             elements_ == other.elements_;
    }
    inline bool operator!=(const Iterator<IterType>& other) const {
      return wrapped_iterator_ != other.wrapped_iterator_ ||
             elements_ != other.elements_;
    }
    IteratorElement operator*() {
      IteratorElement ret_val(wrapped_iterator_->first,
                              elements_->at(wrapped_iterator_->second));
      return ret_val;
    }
    IteratorElement* operator->() {
      saved_elt_.reset(
          new IteratorElement(wrapped_iterator_->first,
                              elements_->at(wrapped_iterator_->second)));
      return saved_elt_.get();
    }

   private:
    IterType wrapped_iterator_;
    const std::vector<EltType>* elements_;
    std::unique_ptr<IteratorElement> saved_elt_;
  };

  typedef Iterator<RepositoryMap::const_iterator> PrimaryIterator;
  typedef Iterator<RepositoryMultiMap::const_iterator> SecondaryIterator; 

  // The extractors vector is *not* made const, because we perform a move
  // operation on the contents of the vector, thereby passing ownership
  // to the repository.
  MemRepository(std::unique_ptr<Extractor> main_extractor,
                std::vector<std::unique_ptr<Extractor>>* extractors) :
      main_extractor_(std::move(main_extractor)) {
    for (uint16_t i = 0; i < extractors->size(); i++) {
      extractors_.push_back(std::move(extractors->at(i)));
    }
    indices_.resize(extractors_.size());
    free_list_.push(0);
  }

  ~MemRepository() {}

  inline uint16_t added_index_count() const {return indices_.size();}

  inline const Extractor& main_extractor() const { return *main_extractor_; }
  inline const Extractor& ith_extractor(int i) const {
    return *(extractors_[i]);
  }

  inline int32_t size() const { return main_index_.size(); }

  grpc::Status Add(const EltType& e) {
    // TODO: Properly guard element to handle multi-threaded access.
    //       This method is broken without these guards. However, below
    //       is the main idea behind the logic of what has to happen.

    // First, extract the primary key from the element. Before going
    // too far, we will check for duplicates.
    std::unique_ptr<Comparable> main_key(main_extractor_->GetKey(e));
    RepositoryMap::iterator it = main_index_.lower_bound(main_key);
    if (it != main_index_.end() && *(it->first) == *main_key) {
      // TODO: Log Error.
      std::stringstream error;
      error << "Cannot add duplicate element with key: (\""
            << main_key->to_string()
            << "\")";
      return grpc::Status(grpc::StatusCode::ALREADY_EXISTS, error.str());
    }

    // Next, extract all the key values to be updated.
    std::vector<std::unique_ptr<Comparable>> added_keys;
    for (uint16_t i = 0; i < extractors_.size(); i++) {
      added_keys.push_back(extractors_[i]->GetKey(e));
    }
   
    // Get the position in the elements_ array from the free_list_.
    // Note that the bottom entry in the free_list_ stack is always
    // the one equal to the total size of the elements_ array. 
    int32_t new_elt_pos = free_list_.top();
    free_list_.pop();
    if (free_list_.empty()) {
      // assert: new_elt_pos == elements_.size()
      elements_.push_back(e);
      free_list_.push(elements_.size());
    }
    else {
      elements_[new_elt_pos] = e;
    }

    main_index_.emplace_hint(it, std::move(main_key), new_elt_pos);
    for (uint16_t i = 0; i < added_keys.size(); i++) {
      indices_[i].emplace(std::move(added_keys[i]), new_elt_pos);
    }

    return grpc::Status::OK;
  }

  grpc::Status Remove(const std::unique_ptr<Comparable>& key) {
    if (main_index_.count(key) == 0) {
      // TODO: Log Warning.
      std::stringstream error;
      error << "Unable to find element with key: (\""
            << key->to_string()
            << "\")";
      return grpc::Status(grpc::StatusCode::NOT_FOUND, error.str());
    }
  
    RepositoryMap::iterator it = main_index_.find(key);
    int32_t elt_pos = it->second; 
    const EltType& elt = elements_[elt_pos];
    main_index_.erase(it);
    for (uint32_t i = 0; i < indices_.size(); i++) {
      std::unique_ptr<Comparable> index_key = extractors_[i]->GetKey(elt);
      RepositoryMultiMap& index = indices_[i];
      grpc::Status result = DeleteFromSecondaryIndex(index, key, i, elt_pos);
      if (!result.ok()) {
        return result;
      }
    }
    free_list_.push(elt_pos);
    return grpc::Status::OK;
  }

  grpc::Status Update(const std::unique_ptr<Comparable>& key,
                      const EltType& new_value) {
    ReplacementMutator<EltType> mutator(new_value);
    std::unique_ptr<Comparable> update_key = main_extractor_->GetKey(new_value);
    return ApplyMutation(key, update_key, &mutator);
  }

  grpc::Status Get(const std::unique_ptr<Comparable>& key, EltType* elt) const {
    RepositoryMap::const_iterator it = main_index_.find(key);
    if (it == main_index_.end()) {
      std::stringstream error;
      error << "Unable to find element with key: (\""
            << key->to_string()
            << "\")";
      return grpc::Status(grpc::StatusCode::NOT_FOUND, error.str());
    }
    int32_t location = it->second;
    *elt = elements_[location];
    return grpc::Status::OK;
  }

  // Warning: make sure that the updated_key would match the result of
  // performing the mutation on the element. If this is incorrect, we
  // could get a data corruption, as the mutation will occur - without
  // a means of reverting back - but will not be completed.
  grpc::Status ApplyMutation(const std::unique_ptr<Comparable>& key,
                             const std::unique_ptr<Comparable>& updated_key,
                             ElementMutatorInterface<EltType>* mutator) {
    // TODO: Consider refactoring this method to make it simpler. Not
    // immediately clear what we can do, but it seems we try the same
    // or similar test comparisons a few times in this method.
    RepositoryMap::iterator it = main_index_.find(key);
    if (it == main_index_.end()) {
      std::stringstream error;
      error << "Unable to find element with key: (\""
            << key->to_string()
            << "\")";
      return grpc::Status(grpc::StatusCode::NOT_FOUND, error.str());
    }

    int32_t location = it->second;
    EltType* element = &(elements_[location]);

    RepositoryMap::iterator new_location = main_index_.lower_bound(updated_key);
    if (new_location != main_index_.end() &&
        (*key != *updated_key) &&
        *(new_location->first) == *updated_key) {
      std::stringstream error;
      error << "There is already an element with the key "
            << updated_key->to_string() << ".";
      return grpc::Status(grpc::StatusCode::ALREADY_EXISTS, error.str());
    }

    // Before applying the mutation, we want to capture the secondary
    // key information. That way, after making the change, we can
    // detect the difference.
    std::vector<std::unique_ptr<Comparable>> prior_keys;
    for (uint16_t i = 0; i < extractors_.size(); i++) {
      prior_keys.push_back((extractors_[i])->GetKey(*element));
    }
    grpc::Status mutate_result = mutator->Mutate(element);

    if (!mutate_result.ok()) {
      return mutate_result;
    }

    std::unique_ptr<Comparable> new_key = main_extractor_->GetKey(*element);

    if (*new_key != *updated_key) {
      // This should *never* happen if we do things properly. Any time we
      // perform a Mutate operation, we should verify beforehand that the
      // mutation will not cause duplicate key violations.
      std::stringstream error;
      error << "Internal error: Applied mutation with wrong update key. "
            << "The expected update key was: (\""
            << updated_key->to_string()
            << "\"), but what was found was: (\""
            << new_key->to_string()
            << "\"). This mismatch will cause data corruption.";
      return grpc::Status(grpc::StatusCode::INTERNAL, error.str());
    }
    
    if (*updated_key != *key) {
      if (it == new_location) {
        // While this is fairly rare, we do need to handle this case. The
        // standard approach of erasing the old element than adding with
        // a hint for the new element gets broken when the old and new
        // reference locations are the same (such as what happens if we
        // update a key without actually changing its over all sort location).
        // The reason is because the delete will invalidate the new location
        // iterator reference.
        auto removed_location = main_index_.erase(it);
        if (removed_location != main_index_.begin()) {
          removed_location--;
        }
        main_index_.emplace_hint(removed_location, std::move(new_key),
                                 location);
      } else {
        main_index_.erase(it);
        if (new_location == main_index_.end()) {
          new_location--;
        }
        main_index_.emplace_hint(new_location, std::move(new_key), location);
      }
    }

    for (uint16_t i = 0; i < indices_.size(); i++) {
      const std::unique_ptr<Extractor>& extractor = extractors_[i];
      std::unique_ptr<Comparable> new_secondary = extractor->GetKey(*element);
      grpc::Status result = UpdateSecondaryIndex(std::move(new_secondary),
                                                 std::move(prior_keys[i]),
                                                 location,
                                                 i);
      if (! result.ok()) {
        return result;
      }
    }

    return grpc::Status::OK;
  }

  StatusEltConstPtrPair NonMutableGet(
      const std::unique_ptr<Comparable>& key) const {
    RepositoryMap::const_iterator it = main_index_.find(key);
    if (it == main_index_.end()) {
      std::stringstream error;
      error << "Unable to find element with key: (\""
            << key->to_string()
            << "\")";
      return StatusEltConstPtrPair(
          grpc::Status(grpc::StatusCode::NOT_FOUND, error.str()),
          nullptr);
    }
    int32_t location = it->second;
    return StatusEltConstPtrPair(grpc::Status::OK, &(elements_[location]));
  }

  PrimaryIterator LowerBound(const std::unique_ptr<Comparable>& key) const {
    PrimaryIterator ret_val(main_index_.lower_bound(key), &elements_);
    return ret_val;
  }

  SecondaryIterator LowerBoundByIndex(const std::unique_ptr<Comparable>& key,
                                      int index_number) const {
    return SecondaryIterator(indices_[index_number].lower_bound(key),
                             &elements_);
  }

  PrimaryIterator primary_begin() const {
    return PrimaryIterator(main_index_.begin(), &elements_);
  }

  const PrimaryIterator primary_end() const {
    return PrimaryIterator(main_index_.end(), &elements_);
  }

  const SecondaryIterator secondary_begin(int index_number) const {
    return SecondaryIterator(indices_[index_number].begin(), &elements_);
  }

  const SecondaryIterator secondary_end(int index_number) const {
    return SecondaryIterator(indices_[index_number].end(), &elements_);
  }
 
 private:
  grpc::Status UpdateSecondaryIndex(
      std::unique_ptr<Comparable> new_key,
      const std::unique_ptr<Comparable>& prior_key,
      int32_t elt_pos,
      int32_t index_number) {
    if (prior_key == new_key) {
      return grpc::Status::OK;
    }
    RepositoryMultiMap& index = indices_[index_number];
    indices_[index_number].emplace(std::move(new_key), elt_pos);
    return DeleteFromSecondaryIndex(index, prior_key, index_number, elt_pos);
  }

  grpc::Status DeleteFromSecondaryIndex(RepositoryMultiMap& index,
                                        const std::unique_ptr<Comparable>& key,
                                        int32_t index_number,
                                        int32_t elt_pos) {
    std::pair<RepositoryMultiMap::iterator,
              RepositoryMultiMap::iterator> range = index.equal_range(key);
    RepositoryMultiMap::iterator lower = range.first;
    RepositoryMultiMap::iterator upper = range.second;
    while (lower != upper && lower->second != elt_pos) {
      lower++;
    }
    if (lower->second != elt_pos) {
      // TODO: Log Error. Error probably caused by multi-threaded access
      // corruption. Since we are not really thread-safe here, we should
      // expect this.
      std::stringstream error;
      error << "Index corruption detected when looking at index (\""
            << std::to_string(index_number)
            << "\") while removing key (\""
            << key->to_string()
            << "\") and expecting element position (\""
            << std::to_string(elt_pos)
            << "\").";
      return grpc::Status(grpc::StatusCode::DATA_LOSS, error.str());
    }
    index.erase(lower);
    return grpc::Status::OK;
  }


  std::vector<EltType> elements_;
  std::unique_ptr<Extractor> main_extractor_;
  RepositoryMap main_index_;
  std::vector<std::unique_ptr<Extractor>> extractors_;
  std::vector<RepositoryMultiMap> indices_;
  std::stack<int32_t> free_list_;
};

} // namespace mem_repository
} // namespace acumio

#endif // AcumioServer_mem_repository_h
