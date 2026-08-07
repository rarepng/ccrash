#pragma once

// i have no idea what im doing but i know i will have to do this for every needed one by checking errors 1 by 1

#include <math.h>
#include <cmath>

#if defined(__CUDA_ARCH__) || defined(__clang__)
__device__ inline int __signbitl(long double a) { return __builtin_signbit((double)a); }
__device__ inline int __signbit(double a) { return __builtin_signbit(a); }
__device__ inline int __isinfl(long double a) { return __builtin_isinf((double)a); }
__device__ inline int __isnanl(long double a) { return __builtin_isnan((double)a); }
__device__ inline int __finitel(long double a) { return __builtin_isfinite((double)a); }
#endif

#undef NV_INT_OR_BOOL
#define NV_INT_OR_BOOL bool

#include_next <crt/math_functions.hpp>
