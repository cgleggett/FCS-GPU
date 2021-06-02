#include "LoadGpuFuncHist.h"
#include "CU_BigMem.h"

CU_BigMem* CU_BigMem::bm_ptr ;


LoadGpuFuncHist::~LoadGpuFuncHist(){
  free(m_hf); 
/*
  hipFree((*m_hf_d).low_edge);
  hipFree((*m_hf_d).h_szs);
  for(unsigned int i=0 ; i< (*m_d_hf).nhist ; ++i ){
    hipFree((*m_hf_d).h_contents[i]);
    hipFree((*m_hf_d).h_borders[i]);
  }
*/
  free(m_hf_d) ;
//  hipFree(m_d_hf) ;

 free(m_hf2d);
/*
 hipFree((*m_hf2d_d).h_bordersx);
 hipFree((*m_hf2d_d).h_bordersy);
 hipFree((*m_hf2d_d).h_contents);
*/
 free(m_hf2d_d);
// hipFree(m_d_hf2d) ;

}


void LoadGpuFuncHist::LD2D() {
  if(! m_hf2d ) {
    std::cout<<"Error Load 2DFunctionHisto " << std::endl ;
    return ;
  }
  
  FH2D * hf_ptr =new FH2D ;
  FH2D  hf = { 0, 0, 0,0, 0 };
    
  
  hf.nbinsx = (*m_hf2d).nbinsx ; 
  hf.nbinsy = (*m_hf2d).nbinsy ; 
  //std::cout << ".....Loading  2DFnctionHists, Size of hist" <<  hf.nbinsx  << "x" << hf.nbinsy << std::endl ;  
  //std::cout << "(*m_hf2d).h_bordersy, pointer " <<  (*m_hf2d).h_bordersy   << std::endl ;  
/*
  gpuQ(hipMalloc((void**)&hf.h_bordersx , (hf.nbinsx+1)*sizeof(float))) ;
  gpuQ(hipMalloc((void**)&hf.h_bordersy , (hf.nbinsy+1)*sizeof(float))) ;
  gpuQ(hipMalloc((void**)&hf.h_contents , (hf.nbinsy*hf.nbinsx)*sizeof(float))) ;
*/

   CU_BigMem * p = CU_BigMem::bm_ptr ;
   
   hf.h_bordersx = (float *) (p->cu_bm_alloc((hf.nbinsx+1)*sizeof(float) )) ;
   hf.h_bordersy = (float *) (p->cu_bm_alloc((hf.nbinsy+1)*sizeof(float) )) ;
   hf.h_contents  = (float *) (p->cu_bm_alloc(hf.nbinsy*hf.nbinsx*sizeof(float) )) ;

  gpuQ(hipMemcpy( hf.h_bordersx, (*m_hf2d).h_bordersx,  (hf.nbinsx+1)*sizeof(float), 
	hipMemcpyHostToDevice)) ;
//  std::cout << "hf.h_bordersy, pointer " <<  hf.h_bordersy  <<  std::endl ;  
  gpuQ(hipMemcpy( hf.h_bordersy, (*m_hf2d).h_bordersy,  (hf.nbinsy+1)*sizeof(float), 
	hipMemcpyHostToDevice)) ;
  gpuQ(hipMemcpy( hf.h_contents, (*m_hf2d).h_contents,  (hf.nbinsx*hf.nbinsy)*sizeof(float), 
	hipMemcpyHostToDevice)) ;


  *(hf_ptr)= hf ;
  m_hf2d_d = hf_ptr ;

//  gpuQ(hipMalloc((void**)&m_d_hf2d , sizeof(FH2D))) ;
  m_d_hf2d = (FH2D *) (p->cu_bm_alloc(sizeof(FH2D))) ; 
  gpuQ(hipMemcpy( m_d_hf2d, m_hf2d_d,   sizeof(FH2D), hipMemcpyHostToDevice)) ;



//std::cout << "LD2D: bdx,y: "<<hf.nbinsx<<"," <<  hf.nbinsy << " memeory: " <<(hf.nbinsx+hf.nbinsy+2+ hf.nbinsy*hf.nbinsx)*sizeof(float) << " M of FH2D str: "<< sizeof(FH2D)  <<std::endl ;

}


void LoadGpuFuncHist::LD() {
// this call  assume  already have Histofuncs set in m_hf 
// this function allocate memory of GPU and deep copy m_hf to m_d_hf 

  if(! m_hf ) {
    std::cout<<"Error Load WiggleHistoFunctions " << std::endl ; 
    return ;
  }

  FHs hf= {0, 0,0,0,0,0 } ;
  hf.s_MaxValue = (*m_hf).s_MaxValue;
  hf.nhist = (*m_hf).nhist;
  unsigned int * h_szs = (*m_hf).h_szs ;    // already allocateded on host ; 
  
   CU_BigMem * p = CU_BigMem::bm_ptr ;
  
// size_t  s=0 ;

  
 // gpuQ(hipMalloc((void**)&hf.low_edge , (hf.nhist+1)*sizeof(float))) ;
   hf.low_edge = (float *) (p->cu_bm_alloc((hf.nhist+1)*sizeof(float) )) ;
  gpuQ(hipMemcpy( hf.low_edge, (*m_hf).low_edge,  (hf.nhist+1)*sizeof(float), 
	hipMemcpyHostToDevice)) ;
//s += (hf.nhist+1)*sizeof(float) ;

//  gpuQ(hipMalloc((void**)&hf.h_szs , hf.nhist*sizeof(unsigned int))) ;
   hf.h_szs = (unsigned int *) (p->cu_bm_alloc(hf.nhist*sizeof(float) )) ;
  gpuQ(hipMemcpy( hf.h_szs, (*m_hf).h_szs,  hf.nhist*sizeof(unsigned int), 
	hipMemcpyHostToDevice)) ;
//s += hf.nhist*sizeof(unsigned int) ;

 // gpuQ(hipMalloc((void**)&hf.h_contents , hf.nhist*sizeof(uint32_t * ))) ;
 // gpuQ(hipMalloc((void**)&hf.h_borders , hf.nhist*sizeof(float * ))) ;

   hf.h_contents = (uint32_t * *) (p->cu_bm_alloc(hf.nhist*sizeof(uint32_t *) )) ;
   hf.h_borders = (float * *) (p->cu_bm_alloc(hf.nhist*sizeof(float *) )) ;
//s += hf.nhist * (sizeof(uint32_t * ) +sizeof(float * )) ;

  uint32_t* * contents_ptr = (uint32_t* *) malloc(hf.nhist*sizeof(uint32_t*)) ;
  float * * borders_ptr = (float* *) malloc(hf.nhist*sizeof(float*)) ;
  
  for( unsigned int i =0 ; i< hf.nhist ; ++i) {

    //  gpuQ(hipMalloc((void**)(contents_ptr+i) ,  h_szs[i]*sizeof(uint32_t))) ;
    //  gpuQ(hipMalloc((void**)&(borders_ptr[i]) ,  (h_szs[i]+1)*sizeof(float))) ;

     contents_ptr[i] = (uint32_t *)  (p->cu_bm_alloc(h_szs[i]*sizeof(uint32_t ) )) ; 
     borders_ptr[i] =  (float * ) (p->cu_bm_alloc((h_szs[i]+1)*sizeof(float ) )) ; 

     gpuQ(hipMemcpy( contents_ptr[i], (*m_hf).h_contents[i],  h_szs[i]*sizeof(uint32_t),
	hipMemcpyHostToDevice)) ;
      gpuQ(hipMemcpy( borders_ptr[i], (*m_hf).h_borders[i],  (h_szs[i]+1) *sizeof(float),
	hipMemcpyHostToDevice)) ;
  //std::cout << ".....Loading  WiggleFunctionHistss, Size of Hists[" << i << "]=" << h_szs[i]<< std::endl ;  
//   s += h_szs[i]*sizeof(uint32_t) +( h_szs[i]+1)*sizeof(float) ;

  }
  
  gpuQ(hipMemcpy( hf.h_contents, contents_ptr ,hf.nhist*sizeof(uint32_t*),hipMemcpyHostToDevice)) ;
  gpuQ(hipMemcpy( hf.h_borders, borders_ptr ,hf.nhist*sizeof(float*),hipMemcpyHostToDevice)) ;

  
 // gpuQ(hipMalloc((void**)&m_d_hf, sizeof(FHs) )) ;
     m_d_hf = (FHs * )( p->cu_bm_alloc(sizeof(FHs ) )) ; 
  gpuQ(hipMemcpy(m_d_hf, &hf, sizeof(FHs), hipMemcpyHostToDevice)) ;


  free(contents_ptr) ;
  free(borders_ptr);

  m_hf_d = &hf ;
//std::cout << "LD1D: nhist: "<<hf.nhist<<"   memeory: " <<s << " M of FHs str: "<< sizeof(FHs)  <<std::endl ;

}