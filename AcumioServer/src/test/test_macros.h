#ifndef AcumioServer_test_macros_h
#define AcumioServer_test_macros_h
//============================================================================
// Name        : test_macros.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Provides common macros for testing - builds off of gtest
//               macros.
//============================================================================
#include <gtest/gtest.h>

#define EXPECT_OK(expr) do { grpc::Status acumio_result; \
                             EXPECT_TRUE((acumio_result = expr).ok()) \
                                 << acumio_result.error_message();} while(0)

#define EXPECT_ERROR(expr, code) do  { EXPECT_EQ(expr.error_code(), \
                                                 code); } while(0)
#endif // AcumioServer_test_macros_h
