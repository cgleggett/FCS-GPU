#include "Rand4Hits.h"
#include <iostream>

#include "Rand4Hits_cpu.cxx"
#include <cstring>


// Kernel for initializing random number generator engines
struct InitRngKernel
{
  template<typename TAcc, typename TExtent, typename TRandEngine>
  ALPAKA_FN_ACC auto operator()(TAcc const& acc                    // current accelerator
				, TExtent const extent             // size of the generator states buffer
				, TRandEngine* const states        // generator states buffer
                                , unsigned long long const seed    // seed number
				) const -> void
  {
    auto const idx = alpaka::getIdx<alpaka::Grid, alpaka::Threads>(acc)[0];
    if(idx < extent[0]) {
      TRandEngine engine(seed + static_cast<unsigned long long>(idx), 0, 0);
      states[idx] = engine;
    }
  }
};


// Kernel for generating random numbers
struct GenerateKernel
{
  template<typename TAcc, typename TExtent>
  ALPAKA_FN_ACC auto operator()(TAcc const& acc                      // current accelerator
				, TExtent const extent               // size of the memory buffer with random numbers
				, RandomEngine<TAcc>* const states   // buffer with generator states
				, float* const cells                 // memory buffer with random numbers
				, size_t numStates                   // size of the memory buffer with generator states
				) const -> void
  {
    /// Index of the current thread. Each thread generates multiple random numbers
    auto const idx = alpaka::getIdx<alpaka::Grid, alpaka::Threads>(acc)[0];
    auto const numGridThreads = alpaka::getWorkDiv<alpaka::Grid, alpaka::Threads>(acc)[0];
    if(idx < numStates) {
      // Each thread is handling one random state
      auto const numWorkers = alpaka::math::min(acc, numGridThreads, static_cast<decltype(numGridThreads)>(numStates));
      RandomEngine<TAcc> engine(states[idx]); // Setup the PRNG using the saved state for this thread.
      alpaka::rand::UniformReal<float> dist; // Setup the random number distribution
      for(uint32_t i = idx; i < extent[0]; i += numWorkers) {
	cells[i] = dist(engine);
      }
      states[idx] = engine; // Save the final PRNG state
    }
  }
};


void Rand4Hits::allocate_simulation( long long /*maxhits*/, unsigned short /*maxbins*/, unsigned short maxhitct,
                                     unsigned long n_cells ) {

  // for args.cells_energy
  m_cellsEnergy = alpaka::allocBuf<CELL_ENE_T, Idx>(alpaka::getDevByIdx<Acc>(0u),Vec{Idx(n_cells)});
  m_cells_energy = alpaka::getPtrNative(m_cellsEnergy);

  // for args.hitcells_E
  m_cellE = alpaka::allocBuf<Cell_E, Idx>(alpaka::getDevByIdx<Acc>(0u),Vec{Idx(maxhitct)});
  m_cell_e = alpaka::getPtrNative(m_cellE);
  m_cell_e_h = (Cell_E*)malloc( maxhitct * sizeof( Cell_E ) );

  // for args.hitcells_E_h and args.hitcells_ct
  m_ct = alpaka::getPtrNative(m_cT);

  printf(" -- R4H ncells: %lu  cells_energy: %p   hitcells_E: %p  hitcells_ct: %p\n",
         n_cells, (void*)m_cells_energy, (void*)m_cell_e, (void*)m_ct);

}

void Rand4Hits::allocateGenMem(size_t num) {
  m_rnd_cpu = new std::vector<float>;
  m_rnd_cpu->resize(num);
  std::cout << "m_rnd_cpu: " << m_rnd_cpu << "  " << m_rnd_cpu->data() << std::endl;
}

Rand4Hits::~Rand4Hits() {
  delete m_rnd_cpu;
}

void Rand4Hits::rd_regen() {
  if ( m_useCPU ) {
    genCPU( 3 * m_total_a_hits );

    BufHost bufHost{alpaka::allocBuf<float, Idx>(alpaka::getDevByIdx<Host>(0u), Vec{Idx(3*m_total_a_hits)})};
    float* const ptrBufHost{alpaka::getPtrNative(bufHost)};
    memcpy(ptrBufHost,m_rnd_cpu->data(),3*m_total_a_hits*sizeof(float));

    alpaka::memcpy(m_queue, m_bufAcc, bufHost);
    alpaka::wait(m_queue);

  } else {

    RandomEngine<Acc>* const ptrBufAccEngine{alpaka::getPtrNative(m_bufAccEngine)};
    WorkDiv workdivGen{alpaka::getValidWorkDiv<Acc>(alpaka::getDevByIdx<Acc>(Idx{0})
						    , Vec(Idx{3*m_total_a_hits})
						    , Vec(Idx{3*m_total_a_hits/NUM_STATES})
						    , false
						    , alpaka::GridBlockExtentSubDivRestrictions::Unrestricted)};

    GenerateKernel genKernel;
    alpaka::exec<Acc>(m_queue
		      , workdivGen
		      , genKernel
		      , Vec{Idx(3*m_total_a_hits)}
		      , ptrBufAccEngine
		      , m_rand_ptr
		      , NUM_STATES);
    alpaka::wait(m_queue);

  }
}

void Rand4Hits::create_gen( unsigned long long seed, size_t num, bool useCPU ) {

  m_bufAcc=alpaka::allocBuf<float, Idx>(alpaka::getDevByIdx<Acc>(0u), Vec{Idx(num)});

  m_useCPU = useCPU;

  if ( m_useCPU ) {
    allocateGenMem( num );
    createCPUGen( seed );
    genCPU( num );

    BufHost bufHost{alpaka::allocBuf<float, Idx>(alpaka::getDevByIdx<Host>(0u), Vec{Idx(num)})};
    float* const ptrBufHost{alpaka::getPtrNative(bufHost)};
    memcpy(ptrBufHost,m_rnd_cpu->data(),num*sizeof(float));

    alpaka::memcpy(m_queue, m_bufAcc, bufHost);
    alpaka::wait(m_queue);

  } else {

    // Initialize RNG engine states
    RandomEngine<Acc>* const ptrBufAccEngine{alpaka::getPtrNative(m_bufAccEngine)};
    WorkDiv workdivEngine{alpaka::getValidWorkDiv<Acc>(alpaka::getDevByIdx<Acc>(Idx{0})
						       , Vec(Idx{NUM_STATES})
						       , Vec(Idx{1})
						       , false
						       , alpaka::GridBlockExtentSubDivRestrictions::Unrestricted)};

    InitRngKernel initRng;
    alpaka::exec<Acc>(m_queue
		      , workdivEngine
		      , initRng
		      , Vec(Idx{NUM_STATES})
		      , ptrBufAccEngine
		      , seed);
    alpaka::wait(m_queue);

    // Generate random numbers
    WorkDiv workdivGen{alpaka::getValidWorkDiv<Acc>(alpaka::getDevByIdx<Acc>(Idx{0})
						    , Vec(Idx{num})
						    , Vec(Idx{num/NUM_STATES})
						    , false
						    , alpaka::GridBlockExtentSubDivRestrictions::Unrestricted)};
    GenerateKernel genKernel;
    alpaka::exec<Acc>(m_queue
		      , workdivGen
		      , genKernel
		      , Vec{Idx(num)}
		      , ptrBufAccEngine
		      , alpaka::getPtrNative(m_bufAcc)
		      , NUM_STATES);
    alpaka::wait(m_queue);

    m_gen = (void*)ptrBufAccEngine;

  }

  m_rand_ptr = alpaka::getPtrNative(m_bufAcc);
}
