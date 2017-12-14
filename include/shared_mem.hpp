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

// @Author Abdullah, Sabir, Wasif
// Header file for the shared memory implementation of RapMap

#ifndef __RAPMAP_SHARED_MEM_HPP__
#define __RAPMAP_SHARED_MEM_HPP__


#include <sys/shm.h>
#include <iostream>
#include <fstream>
#include <errno.h>
#include <ext/stdio_filebuf.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/shm.h>
#include <string>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <vector>
#include <map>

#include <cereal/types/unordered_map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/map.hpp>

#include <dirent.h>
#include <sys/types.h>



namespace shared_mem
{
	const int defaultFileMode = 0666;

	// page size, needed to allocate shared memory
	const int SHM_PAGE_SIZE = 4096;

	// Name of the shared memory, we will use this name + file name to 
	// create different segment name
	extern std::string memName;

	// Map to save the segment size of specific shared memory segment
	extern std::map<std::string, int> shmSegmentToSizeMap;

	// A flag to check if the data is going to be written to or laoded from shared mem
	extern bool isSharedMem;


	// Name of the files that we save in the shared memory
	const std::vector<std::string>fileName = {
		"hash",
        "sa"
	};

	// We allocate extra 4K space in shared memory just to make sure we got
	// enough memory to save the data structure related meta data
	// e.g. std::vector might have some linkege pointer data
	const off_t extraSpaceInSharedMem = 0;
	// @brief initializes a shared memory segment, it creates/opens an
	// existing shared memory segment with the given 'name' parameter
	// @param 'name' name of the shared memroy segment
	// @param 'oflag' is a bit mask created by ORing together exactly one of O_RDONLY
       // or O_RDWR and any of the other flags listed here:

       // O_RDONLY   Open the object for read access.  A shared memory object
       //            opened in this way can be mmap(2)ed only for read
       //            (PROT_READ) access.

       // O_RDWR     Open the object for read-write access.

       // O_CREAT    Create the shared memory object if it does not exist.  The
       //            user and group ownership of the object are taken from the
       //            corresponding effective IDs of the calling process, and
       //            the object's permission bits are set according to the low-
       //            order 9 bits of mode, except that those bits set in the
       //            process file mode creation mask (see umask(2)) are cleared
       //            for the new object.  A set of macro constants which can be
       //            used to define mode is listed in open(2).  (Symbolic
       //            definitions of these constants can be obtained by
       //            including <sys/stat.h>.)

	// @param 'mode' which mode the file is being opened, user level access 
	// @param 'size' size of the shared memory segment
	// @param 'fd' "ouptut param" returns the file descriptor of the shared memory 
	void * initSharedMemory(std::string name, off_t size, int &shm_fd, int oflag = O_CREAT | O_RDWR, mode_t mode = 0666);

	int deinitializeSharedMemory(void *shm_base, int &shm_fd, off_t size);

	// @brief unilinks a shared memory segment with name 'name'
	// if no other process points to the file anymore,
	// this functions removes the shared memory segment from the memory
	// it uses shm_unlink underneath the hood
	// @param 'name' name of the segment to unlink
	int removeSharedMemory(std::string name);
	// int shm_unlink(const char *name);


	// @brief saves a std::vector 'vec' data to shared memory name 'name' 
	// in a serialized binary format
	// uses cereal for serializing
	// @param vec vector to save
	// @param name place to save
	template<typename T, typename A>
	void saveBinaryVector( std::vector<T,A> const& vec, std::string name ) 
	{
		int shmFd;
		size_t dataSize = static_cast<std::size_t>( vec.size() * sizeof(T) );
		std::cerr << "Data size = " << dataSize << std::endl;
		// Size on the shared memory would be the minimum (SHM_PAGE_SIZE)4K size that can
		// hold the vector data
		off_t shmSize = static_cast<off_t>(((dataSize/shared_mem::SHM_PAGE_SIZE) + 1 ) * shared_mem::SHM_PAGE_SIZE);

        void *shmBase = shared_mem::initSharedMemory(name, shmSize, shmFd);

        // @FIXME:
        // Converting a c file descriptor to c++ stream
        // We are using a GNU compiler dependent method, 
        // won't work with other compiler
        __gnu_cxx::stdio_filebuf<char> shmFileBuf(shmFd, std::ios::out);
		{
			std::ostream shmStream(&shmFileBuf);
			// create cereal::BinaryOutputArchive object and save the binary to the shared memory
			cereal::BinaryOutputArchive shmArchive(shmStream);
			// shmArchive(vec);
			shmArchive.saveBinary(vec.data(), dataSize);
			shmStream.flush();
		}
		
		// close the shared memory segment from this process
        shared_mem::deinitializeSharedMemory(shmBase, shmFd, shmSize);

        // save the information about the size of the data in a json file
        // We need this info to open the shared memory for loading the data in the future

        // @Done: We may want to write this data at the end of the indexer process
        // We can use a json file to write the data
        // On the meantime during the indexer process we can save this data in a map

        // @Done: Similarly we can load this map from the json file at the 
        // beginning of the mapping process
        shared_mem::shmSegmentToSizeMap[name] = dataSize;
	}

	// @brief loads a std::vector 'vec' data from shared memory name 'name' 
	// from a serialized binary format to a std::vector
	// uses cereal for de-serializing
	// @param vec vector to load to
	// @param name place to load from
	template<typename T, typename A>
	void loadBinaryVector( std::vector<T,A> & vec, std::string name ) 
	{
		int shmFd;
		size_t dataSize = static_cast<std::size_t>(shared_mem::shmSegmentToSizeMap[name]);
        std::cerr << "Segment name = " << name << std::endl;
        std::cerr << "Data size = " << dataSize << std::endl;
		// size_t dataSize = 151208; //test
		
		int vecSize = dataSize/sizeof(T);
        std::cerr << "Vector size = " << vecSize << std::endl;
		// Size on the shared memory that was allocated to hold the vector data
		off_t shmSize = static_cast<off_t>(((dataSize/shared_mem::SHM_PAGE_SIZE) + 1 ) * shared_mem::SHM_PAGE_SIZE);
		// off_t shmSize = 4096; //just for test purpose

        // void *shmBase = shared_mem::initSharedMemory(name, shmSize, shmFd, O_RDWR);
        void *shmBase = shared_mem::initSharedMemory(name, shmSize, shmFd);

        // @FIXME:
        // Converting a c file descriptor to c++ stream
        // We are using a GNU compiler dependent method, 
        // won't work with other compiler
        __gnu_cxx::stdio_filebuf<char> shmFileBuf(shmFd, std::ios::in);
		{
			std::istream shmStream(&shmFileBuf);
			// create cereal::BinaryOutputArchive object and save the binary to the shared memory
			cereal::BinaryInputArchive shmArchive(shmStream);
			// shmArchive(vec);
			vec.resize(vecSize);
			shmArchive.loadBinary(vec.data(), dataSize);
			// shmArchive.loadBinary(vec.data(), 75604); //test
			
		}
		
		// close the shared memory segment from this process
        // shared_mem::deinitializeSharedMemory(shmBase, shmFd, shmSize);
	}

	// @brief remove the shared memory segments
	// We iterate through the shmSegmentToSizeMap which contains all the shared memory name
	// We might want to keep a const vector which tracks all the possible shared mem segment
	// we create, 
	// Now the the user can give us the shared_mem name prefix and we can iterate through the vector
	// to remove them
	int removeSharedMemoryWithPrefix(std::string prefix);



	// @brief serializes a std map to json archive
	// @param a std map, and name (path to save to)
	// We save this in a file not in shared memory
	template<typename T, typename F>
	void saveJSONMap(std::map<T,F> & map, std::string name) 
	{
        std::ofstream mapStream(name);
        {
            cereal::JSONOutputArchive ar(mapStream);
            ar(CEREAL_NVP(map));
        }
	}

    // @brief deserializes a std map from json archive
    // We save this in a file not in shared memory
    // T&& map
    template<typename T, typename F>
    void loadJSONMap(std::map<T,F> & map, std::string name) 
    {
        std::ifstream mapStream(name);
        {
            cereal::JSONInputArchive ar(mapStream);
            ar(CEREAL_NVP(map));
        }
    }

    // Loads a already saved index to shared memory for future use
    void loadIndexToSharedMem(std::string inputDirName, std::string sharedMemPrefix);

    // Saves a already shared memory index to ouptutDir for future use
    void saveIndexToDisk(std::string outputDirName, std::string sharedMemPrefix);

	void display(char *prog, char *bytes, int n);

}

#endif //__RAPMAP_SHARED_MEM_HPP__
