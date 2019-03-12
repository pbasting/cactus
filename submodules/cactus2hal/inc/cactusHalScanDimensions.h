/*
 * Copyright (C) 2012 by Glenn Hickey (hickey@soe.ucsc.edu)
 *
 * Released under the MIT license, see LICENSE.cactus
 */

#ifndef _HALCACTUSSCANDIMENSIONS_H
#define _HALCACTUSSCANDIMENSIONS_H

#include <string>
#include <fstream>
#include <map>

#include "cactusHalScanner.h"
#include "fastaReader.h"

typedef  std::map<std::string, std::vector<hal::Sequence::Info>* > GenMapType;
/**
 * Scan the .hal file to determine the demensions of each genome (event)
 */
class CactusHalScanDimensions : protected CactusHalScanner
{
public:

   CactusHalScanDimensions();
   ~CactusHalScanDimensions();

   const GenMapType* getDimensionsMap() const;

   void scanDimensions(const std::string& halFilePath,
                       const std::string& fastaFilePath);

   void getSequence(const std::string& genomeName, 
                    const std::string& sequenceName,
                    std::string& outSequence);

protected:

   void scanSequence(CactusHalSequence& sequence);
   void scanTopSegment(CactusHalTopSegment& topSegment);
   void scanBottomSegment(CactusHalBottomSegment& botSegment);
   void scanEndOfFile();

   void resetCurrent();
   void flushCurrentIntoMap();

protected:

   GenMapType _genomeMap;
   std::string _parentGenome;
   std::string _currentGenome;
   hal_size_t _currentSeqLength;
   hal::Sequence::Info _currentInfo;
   FastaReader _faReader;
};

#endif
