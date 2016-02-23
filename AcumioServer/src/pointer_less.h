#ifndef AcumioServer_pointer_less_h
#define AcumioServer_pointer_less_h
//============================================================================
// Name        : pointer_less.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : A less-than operator that works on pointers to objects
//               rather than on the objects themselves. For nullptr arguments,
//               a nullptr is consider less than any other non-nullptr,
//               and two nullptrs are considered equal.
//
//     A *a1 = getSomeAPointer();
//     A *a2 = getSomeOtherAPointer();
//     pointer_less<A> comp;
//     if(comp(a1, a2)) {
//       cout << "a1 == nullptr and a2 != nullptr or *a1 < *a2";
//     } else {
//       cout << "a2 == nullptr or *a1 >= *a2"
//     }
//
// Similarly:
//     std::unique_ptr<A> a1(getSomeNewAPointer());
//     std::unique_ptr<A> a2(getSomeOtherNewAPointer());
//     pointer_less<A> comp;
//     if (comp(a1,a2)) {
//       cout << "a1 is nullptr and a2 is not nullptr or *a1 < *a2";
//     } else {
//       cout << "a2 is nullptr or *a1 >= *a2"
//     }
//
//============================================================================

#include <memory>

namespace acumio {
namespace functional {
template <typename T> struct pointer_less {
  inline bool operator()(const T* left, const T* right) const {
    return (left == nullptr) ? (right != nullptr) :
           (right == nullptr ? false : *left < *right);
  }
  inline bool operator()(const std::unique_ptr<T>& left,
                         const std::unique_ptr<T>& right) const {
    return (left.get() == nullptr) ? (right.get() != nullptr) :
           (right.get() == nullptr ? false : *left < *right);
  }
  inline bool operator()(const T* left, const std::unique_ptr<T>& right) const {
    return (left == nullptr) ? (right.get() != nullptr) :
           (right.get() == nullptr ? false : *left < *right);
  }
  inline bool operator()(const std::unique_ptr<T>& left,
                         const T* right) const {
    return (left.get() == nullptr) ? (right != nullptr) :
           (right == nullptr ? false : *left < *right);
  }
};
} // namespace functional
} // namespace acumio
#endif // AcumioServer_pointer_less_h
