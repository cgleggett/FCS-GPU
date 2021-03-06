/*
  Copyright (C) 2002-2021 CERN for the benefit of the ATLAS collaboration
*/

#include "TROOT.h"

void runShape() {
    gROOT->ProcessLine(".x initTFCSAnalyzer.C");
    gROOT->ProcessLine(".x runTFCS2DParametrizationHistogram.cxx(@DSID@, @DSIDZV0@, @METADATA@, @DIR@, @VER@, @CUTOFF@,  @PLOTDIR@, @DO2DPARAM@, @PHISYMM@, @DOMEANRZ@, @USEMEANRZ@, @DOZVERTEXSTUDIES@ )");
}
