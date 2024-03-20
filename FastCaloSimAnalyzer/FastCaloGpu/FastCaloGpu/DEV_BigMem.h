/*
  Copyright (C) 2002-2021 CERN for the benefit of the ATLAS collaboration
*/

#ifndef DEV_BIGMEM_H
#define DEV_BIGMEM_H

#include <vector>
#include <iostream>

#ifdef USE_ALPAKA
#include "AlpakaDefs.h"
#endif

#ifdef USE_OMPGPU
#include <omp.h>
#endif

class DEV_BigMem {

public:
  DEV_BigMem() : m_ptrs(0), m_seg(0), m_seg_size(0), m_used(0) {};
  DEV_BigMem(size_t s);
  ~DEV_BigMem();

  template <typename T> T *dev_bm_alloc(size_t s) {
    s = s * sizeof(T);
    constexpr size_t ALIGN = 128;
    size_t pad = (ALIGN - s % ALIGN) % ALIGN;
    s += pad;
    m_pad += pad;
    if (s > (m_seg_size - m_used[m_seg]))
      add_seg();
    long *q = (long *)m_ptrs[m_seg];
    int offset = m_used[m_seg] / sizeof(long);
    void *p = (void *)&(q[offset]);
    m_used[m_seg] += ((s + sizeof(long) - 1) / sizeof(long)) * sizeof(long);

    return (T *)p;
  };

  size_t size() const {
    return (m_seg + 1) * m_seg_size;
  };
  size_t used() const {
    return m_seg * m_seg_size + m_used[m_seg];
  };
  size_t lost() const { return m_pad; }

  static DEV_BigMem *bm_ptr;

private:
  void add_seg();

  std::vector<void *> m_ptrs; // points to each allocated segment
  int m_seg{ 0 };             // do we need ? it's current size of mptrs -1 ;
  size_t m_seg_size{ 0 };     // size of each allocation
  std::vector<size_t> m_used; // used memory in each allocated segment
  size_t m_pad{ 0 };          // total space lost to padding

#ifdef USE_ALPAKA
  std::vector<BufAccChar> m_bufs;
#endif

#ifdef USE_OMPGPU
  int m_default_device = omp_get_default_device();
  int m_initial_device = omp_get_initial_device();
  std::size_t m_offset = 0;
#endif
};

#endif
