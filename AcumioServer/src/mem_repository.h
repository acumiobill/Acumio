#ifndef AcumioServer_mem_repository_h
#define AcumioServer_mem_repository_h
//============================================================================
// Name        : mem_repository.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Provides templated multi-threaded access to an in-memory
//               repository.
//============================================================================

#include <stdint.h>
#include <map>
#include <stack>
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
class MemRepository {
 public:
  typedef KeyExtractorInterface<EltType> Extractor;
  typedef std::map<std::unique_ptr<Comparable>, int32_t,
                   acumio::functional::pointer_less<Comparable>> RepositoryMap;

  typedef std::multimap<std::unique_ptr<Comparable>, int32_t,
                        acumio::functional::pointer_less<Comparable>>
      RepositoryMultiMap;

  typedef std::pair<const std::unique_ptr<Comparable>&, const EltType>
      IteratorElement;

  template <typename IterType>
  class Iterator :
      public std::iterator<std::bidirectional_iterator_tag, IteratorElement> {
   public:
    Iterator<IterType>() {}
    Iterator<IterType>(IterType wrapped_iterator,
                       const std::vector<EltType>* elements) :
        wrapped_iterator_(wrapped_iterator), elements_(elements) {}
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
    inline Iterator<IterType>& operator++(int) {
      Iterator<IterType> tmp(*this);
      wrapped_iterator_++;
      return tmp;
    }
    inline Iterator<IterType>& operator--(int) {
      Iterator<IterType> tmp(*this);
      wrapped_iterator_--;
      return tmp;
    }
    inline bool operator==(const Iterator<IterType>& other) {
      return wrapped_iterator_ == other.wrapped_iterator_ &&
             elements_ == other.elements_;
    }
    inline bool operator!=(const Iterator<IterType>& other) {
      return wrapped_iterator_ != other.wrapped_iterator_ ||
             elements_ != other.elements_;
    }
    IteratorElement operator*() {
      IteratorElement ret_val(wrapped_iterator_->first,
                              elements_->at(wrapped_iterator_->second));
      return ret_val;
    }

   private:
    IterType wrapped_iterator_;
    const std::vector<EltType>* elements_; 
  };

  typedef Iterator<RepositoryMap::const_iterator> PrimaryIterator;
  typedef Iterator<RepositoryMultiMap::const_iterator> SecondaryIterator; 

  MemRepository(std::unique_ptr<Extractor> main_extractor,
                std::vector<std::unique_ptr<Extractor>>* extractors) :
      main_extractor_(std::move(main_extractor)) {
    for (uint32_t i = 0; i < extractors->size(); i++) {
      extractors_.push_back(std::move(extractors->at(i)));
    }
    indices_.resize(extractors_.size());
    free_list_.push(0);
  }

  ~MemRepository() {}

  inline uint16_t added_index_count() const {return indices_.size();}

  inline const Extractor& main_extractor() const { return main_extractor_; }
  inline const Extractor& ith_extractor(int i) const { return extractors_[i]; }

  grpc::Status Add(const EltType& e) {
    // TODO: Properly guard element to handle multi-threaded access.
    //       This method is broken without these guards. However, below
    //       is the main idea behind the logic of what has to happen.

    // First, extract the primary key from the element. Before going
    // too far, we will check for duplicates.
    std::unique_ptr<Comparable> main_key(main_extractor_->GetKey(e));
    RepositoryMap::iterator it = main_index_.lower_bound(main_key);
    if (*(it->first) == *main_key) {
      // TODO: Log Error.
      std::string error = "Cannot add duplicate element with key: " +
                           main_key->to_string();
      return grpc::Status(grpc::StatusCode::ALREADY_EXISTS, error);
    }

    // Next, extract all the key values to be updated.
    std::vector<std::unique_ptr<Comparable>> added_keys;
    for (uint32_t i = 0; i < extractors_.size(); i++) {
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
    for (uint32_t i = 0; i < added_keys.size(); i++) {
      indices_[i].emplace(std::move(added_keys[i]), new_elt_pos);
    }

    return grpc::Status::OK;
  }

  grpc::Status Remove(const std::unique_ptr<Comparable>& key) {
    if (main_index_.count(key) == 0) {
      // TODO: Log Warning.
      std::string error = "Unable to find element with key: " +
                           key->to_string();
      return grpc::Status(grpc::StatusCode::NOT_FOUND, error);
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
    // First, get a handle to entry by main key. If unable to find it,
    // we obviously cannot perform an update.
    RepositoryMap::iterator it = main_index_.find(key);
    if (it == main_index_.end()) {
      std::string error = "Unable to update element with key: " +
                          key->to_string() +
                          ". Element not found.";
      return grpc::Status(grpc::StatusCode::NOT_FOUND, error);
    }

    int32_t elt_pos = it->second;
    EltType prior_value = elements_[elt_pos];
    elements_[elt_pos] = new_value;
    std::unique_ptr<Comparable> new_key(main_extractor_->GetKey(new_value));
    if (*new_key != *key) {
       main_index_.erase(it);
       main_index_.emplace_hint(it, std::move(new_key), elt_pos);
    }

    for (uint32_t i = 0; i < indices_.size(); i++) {
      grpc::Status result =
          UpdateSecondaryIndex(new_value, prior_value, elt_pos, i);
      if (! result.ok()) {
        return result;
      }
    }
    return grpc::Status::OK;
  }

  grpc::Status Get(const std::unique_ptr<Comparable>& key, EltType* elt) const {
    RepositoryMap::const_iterator it = main_index_.find(key);
    if (it == main_index_.end()) {
      std::string error = "Unable to find element with key: " +
                          key->to_string();
      return grpc::Status(grpc::StatusCode::NOT_FOUND, error);
    }
    int32_t location = it->second;
    *elt = elements_[location];
    return grpc::Status::OK;
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
 
 private:
  grpc::Status UpdateSecondaryIndex(const EltType& new_value,
                                    const EltType& prior_value,
                                    int32_t elt_pos,
                                    int32_t index_number) {
    const std::unique_ptr<Extractor>& extractor = extractors_[index_number];
    std::unique_ptr<Comparable> prior_key = extractor->GetKey(prior_value);
    std::unique_ptr<Comparable> new_key = extractor->GetKey(new_value);
    if (*prior_key == *new_key) {
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
      std::string error = "Index corruption detected when looking at index " +
          std::to_string(index_number) + " while removing key " +
          key->to_string() + " and expecting element position " +
          std::to_string(elt_pos) + ".";
      return grpc::Status(grpc::StatusCode::DATA_LOSS, error);
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
