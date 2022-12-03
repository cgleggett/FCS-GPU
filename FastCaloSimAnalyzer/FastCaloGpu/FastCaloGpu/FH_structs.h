/*
  Copyright (C) 2002-2023 CERN for the benefit of the ATLAS collaboration
*/

#ifndef FH_STRUCT_H
#define FH_STRUCT_H

typedef struct FHs {
        uint32_t s_MaxValue ;
        float * low_edge ;
        unsigned int nhist;
        unsigned int * h_szs ;
        uint32_t * * h_contents ;
        float * * h_borders ;
        } FHs  ;

typedef struct FH2D {
        int nbinsx ;
        int nbinsy ;
        float * h_bordersx ;
        float * h_bordersy ;
        float * h_contents ;
        } FH2D ;

#endif
