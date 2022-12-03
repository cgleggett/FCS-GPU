#ifndef CU_BIGMEM_H
#define CU_BIGMEM_H

#include "gpuQ.h"
#include <vector>

class CU_BigMem {

public:
  CU_BigMem() : m_seg_size(0), m_ptrs(0), m_used(0), m_seg(0){};
  CU_BigMem(size_t s) { // initialize to one seg with size s
    void *p;
    m_seg_size = s;
    gpuQ(hipMalloc(&p, m_seg_size));
    m_ptrs.push_back(p);
    //  bm_ptr = self ;
    m_seg = 0;
    m_used.push_back(0);
  };

  ~CU_BigMem() {
    for (int i = 0; i < m_ptrs.size(); i++)
      gpuQ(hipFree(m_ptrs[i]));
  };

  template <typename T> T *cu_bm_alloc(size_t s) {
    s = s * sizeof(T);
    
    // pad allocations to align to 128 byte boundaries for GPU
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

  size_t size() const { return (m_seg + 1) * m_seg_size; };
  size_t used() const { return m_seg * m_seg_size + m_used[m_seg]; };
  size_t lost() const { return m_pad; }

  static CU_BigMem *bm_ptr;

private:
  void add_seg() {
    void *p;
    gpuQ(hipMalloc((void **)&p, m_seg_size));
    m_ptrs.push_back(p);
    m_seg++;
    m_used.push_back(0);
  };

  std::vector<void *> m_ptrs; // points to each allocated segment
  int m_seg;                  // do we need ? it's current size of mptrs -1 ;
  size_t m_seg_size;          // size of each allocation
  std::vector<size_t> m_used; // used memory in each allocated segment
  size_t m_pad{0};            // total space lost to padding
};

#endif
