#pragma once

#ifdef USE_OMP
#include <omp.h>
static const bool omp_enable = true;
#else
static const bool omp_enable = false;
#endif

inline int get_max_threads() {
#ifdef USE_OMP
  return omp_get_max_threads();
#else
  return 1;
#endif
}

inline int get_thread_num() {
#ifdef USE_OMP
  return omp_get_thread_num();
#else
  return 0;
#endif
}
