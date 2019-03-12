/*
 * Copyright (C) 2012 by Glenn Hickey (hickey@soe.ucsc.edu)
 *
 * Released under the MIT license, see LICENSE.txt
 */

#ifndef _ALLTESTS_H
#define _ALLTESTS_H

extern "C" {
#include "CuTest.h"
}

CuSuite *CactusHalScanDimensionsTestSuite();
CuSuite *cactusHalScannerTestSuite();

#endif
