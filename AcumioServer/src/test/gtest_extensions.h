#ifndef AcumioServer_gtest_extensions_h
#define AcumioServer_gtest_extensions_h
//============================================================================
// Name        : gtest_extensions.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Provides extension tools to the gtest framework including
//               specializations for using grpc and doing list comparisons.
//               Test classes can choose to include this file rather than
//               directly relying on gtest/gtest.h.
//============================================================================
#include <gtest/gtest.h>

#include <grpc++/grpc++.h>
#include <vector>

#define EXPECT_OK(expr) do { grpc::Status acumio_result; \
                             EXPECT_TRUE((acumio_result = expr).ok()) \
                                 << acumio_result.error_message();} while(0)

#define EXPECT_ERROR(expr, code) do  { EXPECT_EQ(expr.error_code(), \
                                                 code); } while(0)

namespace acumio {
namespace test {
template <class IterType>
grpc::Status CompareStringVectorToIterator(std::vector<std::string> v,
                                           const IterType& begin,
                                           const IterType& end) {
  IterType it = begin;
  uint32_t vector_pos = 0;
  for (; it != end && vector_pos < v.size(); it++, vector_pos++) {
    std::string iter_string = it->first->to_string();
    if (iter_string != v[vector_pos]) {
      return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                          iter_string + " != " + v[vector_pos]);
    }
  }
  if (it != end) {
    return grpc::Status(grpc::StatusCode::OUT_OF_RANGE,
                        "Reached end of vector before end of iterator.");
  }
  if (vector_pos != v.size()) {
    return grpc::Status(grpc::StatusCode::OUT_OF_RANGE,
                        "Reached end of iterator before end of vector.");
  }
  return grpc::Status::OK;
}

} // namespace test
} // namespace acumio
#endif // AcumioServer_gtest_extensions_h
