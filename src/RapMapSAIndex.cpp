//
// RapMap - Rapid and accurate mapping of short reads to transcriptomes using
// quasi-mapping.
// Copyright (C) 2015, 2016 Rob Patro, Avi Srivastava, Hirak Sarkar
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
#include "BooMap.hpp"
#include "FrugalBooMap.hpp"
#include "RapMapSAIndex.hpp"
#include "IndexHeader.hpp"
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>

#include <iostream>
#include <map>
#include <vector>


#include "shared_mem.hpp"

#include <future>
#include <thread>

/*
void set_empty_key(spp::sparse_hash_map<uint64_t,
                       rapmap::utils::SAInterval<IndexT>,
		   rapmap::utils::KmerKeyHasher>& khash,
		   uint64_t k) {
}

void set_empty_key(spp::sparse_hash_map<uint64_t,
                       rapmap::utils::SAInterval<IndexT>,
		   rapmap::utils::KmerKeyHasher>& khash,
		   uint64_t k) {
}
*/

    // Set the SA and text pointer if this is a perfect hash
template <typename IndexT>
void setPerfectHashPointers(RegHashT<uint64_t,
                            rapmap::utils::SAInterval<IndexT>,
                            rapmap::utils::KmerKeyHasher>& khash, std::vector<IndexT>& SA, std::string& seq) {
    // do nothing
}

template <typename IndexT>
void setPerfectHashPointers(PerfectHashT<uint64_t,
                            rapmap::utils::SAInterval<IndexT>>& khash, std::vector<IndexT>& SA, std::string& seq) {
    khash.setSAPtr(&SA);
    khash.setTextPtr(seq.c_str(), seq.length());
}

// These are **free** functions that are used for loading the
// appropriate type of hash.
template <typename IndexT>
bool loadHashFromIndex(const std::string& indexDir,
                       RegHashT<uint64_t,
                       rapmap::utils::SAInterval<IndexT>,
                       rapmap::utils::KmerKeyHasher>& khash) 
{
    // @CSE549 load the hash from the shared mem 
    // laod the two vectors and make the hash
    /*if (shared_mem::isSharedMem)
    {
        std::vector<uint64_t> hashKey;
        std::vector<rapmap::utils::SAInterval<IndexT>> hashVal;
        {
          // logger->info("loading hash info");
          // ScopedTimer timer;
          std::cerr << "Inside SAIndex - shared_mem name = " << shared_mem::memName << std::endl;
          shared_mem::loadBinaryVector(hashKey, shared_mem::memName + "hashkey");
          shared_mem::loadBinaryVector(hashVal, shared_mem::memName + "hashval");

          
          for (int i = 0; i < 10; ++i)
          {
            std::cerr << hashKey[i] << "-" << hashVal[i].begin_ << "-" << hashVal[i].end_ << std::endl;
          }
          // recreating the map from the vectors
          for (int i = 0; i < hashKey.size(); ++i)
          {
            khash[hashKey[i]] = hashVal[i];
          }

          // removes the sharedm mem segment after  each run of maper
          // @TODO: remove this later
          shared_mem::removeSharedMemoryWithPrefix(shared_mem::memName);
        }
    }*/
    if (shared_mem::isSharedMem)
    {
      int shmFd;
      size_t dataSize = static_cast<std::size_t>( shared_mem::shmSegmentToSizeMap[(shared_mem::memName + "hash")] );
      std::cerr << "Data size = " << dataSize << std::endl;
      // Size on the shared memory would be the minimum (SHM_PAGE_SIZE)4K size that can
      // hold the vector data
      off_t shmSize = static_cast<off_t>(((dataSize/shared_mem::SHM_PAGE_SIZE) + 1 ) * shared_mem::SHM_PAGE_SIZE);

      void *shmBase = shared_mem::initSharedMemory((shared_mem::memName + "hash"), shmSize, shmFd);

      // @FIXME:
      // Converting a c file descriptor to c++ stream
      // We are using a GNU compiler dependent method, 
      // won't work with other compiler
      __gnu_cxx::stdio_filebuf<char> shmFileBuf(shmFd, std::ios::in);
      {
          std::istream shmStream(&shmFileBuf);
          // create cereal::BinaryOutputArchive object and save the binary to the shared memory
          // ScopedTimer timer;
          std::cerr << "saving hash to disk . . . ";
          khash.unserialize(typename spp_utils::pod_hash_serializer<uint64_t, rapmap::utils::SAInterval<IndexT>>(),
                  &shmStream);
          std::cerr << "done\n";
          // shmStream.flush();
      }

      // test
      /*int k=0;
      for (auto it = khash.begin(); it!= khash.end(); it++)
      {
        if (k == 5)
        {
          break;
        }
        std::cerr << it->first << "----" << it->second.begin_ << std::endl;
        k++;
      }*/
    }
    else
    {
        std::ifstream hashStream(indexDir + "hash.bin", std::ios::binary);
        khash.unserialize(typename spp_utils::pod_hash_serializer<uint64_t, rapmap::utils::SAInterval<IndexT>>(),
        &hashStream);
    }
      
    return true;
}

template <typename IndexT>
bool loadHashFromIndex(const std::string& indexDir,
		       PerfectHashT<uint64_t, rapmap::utils::SAInterval<IndexT>> & h) 
{
    
    // @CSE549 load the hash from the shared mem 
    /*if (shared_mem::isSharedMem)
    {
        std::string hashBase = shared_mem::memName + "hashinfo";
        // std::cout << 
        h.load(hashBase, shared_mem::isSharedMem);
        auto it = h.begin();
        std::cerr << it->first << "----" << it->second.begin_ << std::endl;
        /*int k=0;
        for (auto it = h.begin(); it!= h.end(); ++it)
        {
          if (k == 5)
          {
            break;
          }
          std::cerr << it->first << "----" << it->second.begin_ << std::endl;
          k++;
        }
    }
    else*/
    {
        std::string hashBase = indexDir + "hash_info";
        h.load(hashBase);
    }
    return true;
}

template <typename IndexT, typename HashT>
RapMapSAIndex<IndexT, HashT>::RapMapSAIndex() {}

// Given a position, p, in the concatenated text,
// return the corresponding transcript
template <typename IndexT, typename HashT>
IndexT RapMapSAIndex<IndexT, HashT>::transcriptAtPosition(IndexT p) {
    return rankDict->rank(p);
}

template <typename IndexT, typename HashT>
bool RapMapSAIndex<IndexT, HashT>::load(const std::string& indDir) {

    auto logger = spdlog::get("stderrLog");
    size_t n{0};

    IndexHeader h;
    std::ifstream indexStream(indDir + "header.json");
    {
      cereal::JSONInputArchive ar(indexStream);
      ar(h);
    }
    indexStream.close();
    uint32_t idxK = h.kmerLen();
    rapmap::utils::my_mer::k(idxK);

    // This part takes the longest, so do it in it's own asynchronous task
    std::future<bool> loadingHash = std::async(std::launch::async, [this, logger, indDir]() -> bool {
	return loadHashFromIndex(indDir, khash);
    });

    if (shared_mem::isSharedMem)
    {
      logger->info("Loading Suffix Array from shared_mem");
      shared_mem::loadBinaryVector(SA,(shared_mem::memName + "sa"));
      std::cerr << "SA vector size = " << SA.size() << "element 0 = " << SA[0] << std::endl;
    }
    else
    {
      std::ifstream saStream(indDir + "sa.bin");
      {
          logger->info("Loading Suffix Array ");
          cereal::BinaryInputArchive saArchive(saStream);
          saArchive(SA);
          //saArchive(LCP);
      }
      saStream.close();
    }
    

    std::ifstream seqStream(indDir + "txpInfo.bin");
    {
        logger->info("Loading Transcript Info ");
        cereal::BinaryInputArchive seqArchive(seqStream);
        seqArchive(txpNames);
        seqArchive(txpOffsets);
        //seqArchive(positionIDs);
        seqArchive(seq);
        seqArchive(txpCompleteLens);
    }
    seqStream.close();

    /*
       std::ifstream rsStream(indDir + "rsdSafe.bin", std::ios::binary);
       {
       logger->info("Loading Rank-Select Data");
       rankDictSafe.Load(rsStream);
       }
       rsStream.close();
       */
    std::string rsFileName = indDir + "rsd.bin";
    FILE* rsFile = fopen(rsFileName.c_str(), "r");
    {
        logger->info("Loading Rank-Select Bit Array");
        bitArray.reset(bit_array_create(0));
        if (!bit_array_load(bitArray.get(), rsFile)) {
            logger->error("Couldn't load bit array from {}!", rsFileName);
            std::exit(1);
        }
        logger->info("There were {} set bits in the bit array", bit_array_num_bits_set(bitArray.get()));
        rankDict.reset(new rank9b(bitArray->words, bitArray->num_of_bits));
    }
    fclose(rsFile);

    {
        logger->info("Computing transcript lengths");
        txpLens.resize(txpOffsets.size());
        if (txpOffsets.size() > 1) {
            for(size_t i = 0; i < txpOffsets.size() - 1; ++i) {
                auto nextOffset = txpOffsets[i+1];
                auto currentOffset = txpOffsets[i];
                txpLens[i] = (nextOffset - 1) - currentOffset;
            }
        }
        // The last length is just the length of the suffix array - the last offset
        txpLens[txpOffsets.size()-1] = (SA.size() - 1) - txpOffsets[txpOffsets.size() - 1];
    } 

    logger->info("Waiting to finish loading hash");
    loadingHash.wait();
    auto hashLoadRes = loadingHash.get();
    if (!hashLoadRes) {
        logger->error("Failed to load hash!");
        std::exit(1);
    }
    // Set the SA and text pointer if this is a perfect hash
    setPerfectHashPointers(khash, SA, seq); 
    logger->info("Done loading index");
    return true;
}

template class RapMapSAIndex<int32_t,  RegHashT<uint64_t,
                      rapmap::utils::SAInterval<int32_t>,
                      rapmap::utils::KmerKeyHasher>>;
template class RapMapSAIndex<int64_t,  RegHashT<uint64_t,
                      rapmap::utils::SAInterval<int64_t>,
                      rapmap::utils::KmerKeyHasher>>;
template class RapMapSAIndex<int32_t, PerfectHashT<uint64_t, rapmap::utils::SAInterval<int32_t>>>;
template class RapMapSAIndex<int64_t, PerfectHashT<uint64_t, rapmap::utils::SAInterval<int64_t>>>;
