//
// RapMap - Rapid and accurate mapping of short reads to transcriptomes using
// quasi-mapping.
// Copyright (C) 2015, 2016, 2017 Rob Patro, Avi Srivastava, Hirak Sarkar
//
// This file is part of RapMap.
//
// RapMap is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RapMap is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RapMap.  If not, see <http://www.gnu.org/licenses/>.
//
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <random>
#include <type_traits>
#include <unordered_map>
#include <map>
#include <vector>

#include "tclap/CmdLine.h"


#include "IndexHeader.hpp"

#include <chrono>

#include "shared_mem.hpp"

#include "RapMapSharedMemIndexLoader.hpp"


int rapMapSharedMemIndexLoader(int argc, char* argv[]) 
{
  std::cerr << "RapMap shared_mem index loader and saver\n";

  TCLAP::CmdLine cmd("RapMap shared_mem index loader and saver");
  TCLAP::SwitchArg load("l", "loadtosharedmem",
                                           "Load from a file to shared mem",
                                           false);
  TCLAP::SwitchArg save("s", "savetodisk",
                                           "save to a disk from shared mem",
                                           false);
  TCLAP::SwitchArg remove_("r", "removefromsharedmem",
                                           "remove the segment from shared memory",
                                           false);
  TCLAP::ValueArg<std::string> index(
      "i", "index", "The location from where the index should be loaded or written to", true, "",
      "path");
  TCLAP::ValueArg<std::string> sharedMem("y", "sharedMemory", "Name of shared memory location", true, "","name string");
  
  /*
  TCLAP::SwitchArg perfectHash(
      "f", "frugalPerfectHash", "Use a frugal variant of the perfect hash --- "
                          "this will considerably slow construction, and somewhat slow lookup, but "
                          "hash construction and the subsequent mapping will require the least memory."
      false);
  */
  
  cmd.add(load);
  cmd.add(save);
  cmd.add(remove_);
  cmd.add(index);
  cmd.add(sharedMem);
  
  cmd.parse(argc, argv);

  // @CSE549
  bool isLoad = load.getValue();
  bool isSave = save.getValue();
  bool isRemove = remove_.getValue();
  std::string fileDir = index.getValue();

  if (!sharedMem.getValue().empty())
  {
    shared_mem::memName=sharedMem.getValue();
    shared_mem::isSharedMem = true;
  }

  std::cerr << "Shared memName = " << shared_mem::memName << std::endl;

  if (isLoad)
  {
  	shared_mem::loadIndexToSharedMem(fileDir, shared_mem::memName);
    std::cerr << "loading index to shared memory \n";
  }
  else if (isSave)
  {
    shared_mem::loadJSONMap(shared_mem::shmSegmentToSizeMap, shared_mem::memName + "quasi_shm_segment_size.json");
  	shared_mem::saveIndexToDisk(fileDir, shared_mem::memName);
    std::cerr << "saving index to disk \n";
  }
  else if (isRemove)
  {
    shared_mem::removeSharedMemoryWithPrefix(shared_mem::memName);
  }
  else
  {
  	std::cerr << "Please select a method, load or save, see help for more info \n";
  }

  return 0;
}
