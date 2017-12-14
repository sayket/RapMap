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
// Implementation file for the shared memory implementation of RapMap

#include "shared_mem.hpp"

namespace shared_mem
{
	// Defining the global variables
	std::string memName;
	bool isSharedMem = false;
	std::map<std::string, int> shmSegmentToSizeMap;
	


	// Error message macro
	#define SHM_ERROR_MSG(func, err_msg, err_no) std::cerr << "SHARED_MEM_ERROR in: " << \
					 func << ": " << err_msg << " Error no:" << err_no << std::endl

	// success message macro
	#define SHM_SUCCESS_MSG(func, msg) std::cerr << "SHARED_MEM_SUCCESS in: " << \
					 func << ": " << msg << std::endl


	
	void * initSharedMemory(std::string name, off_t size, int &shm_fd, int oflag, mode_t mode)
	{
		int shmFD;		// file descriptor, from shm_open()
  		void *shmBase = NULL;	// base address, from mmap()
  		off_t allocateSize = size + extraSpaceInSharedMem;
  		/* create the shared memory segment as if it was a file */
		shmFD = shm_open(name.c_str(), O_CREAT | O_RDWR, mode);
		if (shmFD == -1) 
		{
			// std::cerr << "Unable to initialize Shared memory segment: " << strerror(errno) << std::endl;
			SHM_ERROR_MSG("initSharedMemory", "Unable to initialize Shared memory segment", errno);
			goto out;
		}

		/* configure the size of the shared memory segment */
  		ftruncate(shmFD, allocateSize);

  		/* map the shared memory segment to the address space of the process */
		shmBase = (char *)mmap(0, allocateSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmFD, 0);
		if (shmBase == MAP_FAILED) 
		{
			// std::cerr << "Unable to Map the Shared memory segment: " << strerror(errno) << std::endl;
			SHM_ERROR_MSG("initSharedMemory", "Unable to Map the Shared memory segment", errno);
			// close and shm_unlink
			shm_unlink(name.c_str());
			goto out;
		}

		SHM_SUCCESS_MSG("initSharedMemory", "shared_mem segment " + name + " created");

	out:
		shm_fd = shmFD;
		return shmBase;

	}

	int deinitializeSharedMemory(void *shm_base, int &shm_fd, off_t size)
	{
		int ret;
		off_t deallocateSize = size + extraSpaceInSharedMem;
		/* remove the mapped memory segment from the address space of the process */
		ret=munmap(shm_base, deallocateSize);
		if (ret == -1) 
		{
			SHM_ERROR_MSG("deinitializeSharedMemory", "Unmap failed", errno);
			goto out;
		}

		/* close the shared memory segment as if it was a file */
		ret = close(shm_fd);
		if (ret == -1) 
		{
			SHM_ERROR_MSG("deinitializeSharedMemory", "Close failed", errno);
			goto out;
		}

		SHM_SUCCESS_MSG("deinitializeSharedMemory", "shared_mem segment unmapped from process private address space");
	out:
		return ret;

	}

	int removeSharedMemory(std::string name)
	{
		int ret = shm_unlink(name.c_str());
		if (ret == -1)
		{
			SHM_ERROR_MSG("removeSharedMemory", "Unable to unlink the Shared memory segment", errno);
		}
		SHM_SUCCESS_MSG("removeSharedMemory", name + " removed");
		return ret;
	}

	int removeSharedMemoryWithPrefix(std::string prefix)
	{
		int ret;
		for(auto f: fileName)
		{
			if((ret=removeSharedMemory(prefix + f) == -1))
				break;
		}

		return ret;
	}



	void loadIndexToSharedMem(std::string inputDirName, std::string sharedMemPrefix)
	{
		struct dirent *entry;
	    DIR *dir = opendir(inputDirName.c_str());
	    if (dir == NULL) {
	        return;
	    }

	    while ((entry = readdir(dir)) != NULL) 
	    {
	    	if (std::string(entry->d_name) == "hash.bin")
	    	{

	    		std::ifstream infile (inputDirName + std::string(entry->d_name), std::ifstream::binary);
				// get size of file
				infile.seekg (0,infile.end);
				long size = infile.tellg();
				infile.seekg (0);


				std::string name = sharedMemPrefix + "hash";
				shared_mem::shmSegmentToSizeMap[name] = size;
				
				int shmFd;
				size_t dataSize = static_cast<std::size_t>(size);
				// Size on the shared memory that was allocated to hold the vector data
				off_t shmSize = static_cast<off_t>(((dataSize/shared_mem::SHM_PAGE_SIZE) + 1 ) * shared_mem::SHM_PAGE_SIZE);
		        void *shmBase = shared_mem::initSharedMemory(name, shmSize, shmFd);
		        // read content of infile to shared memory
				infile.read ((char *)shmBase,size);
	    	}
	    	else if (std::string(entry->d_name) == "sa.bin")
	    	{
	    		std::ifstream infile (inputDirName + std::string(entry->d_name), std::ifstream::binary);
				// get size of file
				infile.seekg (0,infile.end);
				uint64_t size = infile.tellg();
				infile.seekg (0);

				std::string name = sharedMemPrefix + "sa";
				shared_mem::shmSegmentToSizeMap[name] = size;

				// std::cerr << "Size == " << entry->d_name << "-" << size << std::endl;
				// std::cerr << "Size == " << name << "-" << shared_mem::shmSegmentToSizeMap[name] << std::endl;

				int shmFd;
				size_t dataSize = static_cast<std::size_t>(size);
		        // std::cerr << "Segment name = " << name << std::endl;
		        // std::cerr << "Data size = " << dataSize << std::endl;
				// Size on the shared memory that was allocated to hold the vector data
				off_t shmSize = static_cast<off_t>(((dataSize/shared_mem::SHM_PAGE_SIZE) + 1 ) * shared_mem::SHM_PAGE_SIZE);
		        void *shmBase = shared_mem::initSharedMemory(name, shmSize, shmFd);
		        // read content of infile to shared memory
				infile.read ((char *)shmBase,size);
	    	}
	    }

	    shared_mem::saveJSONMap(shared_mem::shmSegmentToSizeMap, shared_mem::memName + "quasi_shm_segment_size.json");
	    closedir(dir);
		
	}


	void saveIndexToDisk(std::string outputDirName, std::string sharedMemPrefix)
	{
		std::string segmentName = (sharedMemPrefix + "hash");
    	{
    		std::ofstream outfile ((outputDirName + "/" + "hash.bin"),std::ofstream::binary);

			uint64_t size = shared_mem::shmSegmentToSizeMap[(sharedMemPrefix + "hash")];

			int shmFd;
			size_t dataSize = static_cast<std::size_t>(size);
	        std::cerr << "Segment name = " << segmentName << std::endl;
	        std::cerr << "Data size = " << dataSize << std::endl;
			// Size on the shared memory that was allocated to hold the vector data
			off_t shmSize = static_cast<off_t>(((dataSize/shared_mem::SHM_PAGE_SIZE) + 1 ) * shared_mem::SHM_PAGE_SIZE);
	        void *shmBase = shared_mem::initSharedMemory(segmentName, shmSize, shmFd);
	        // read content of infile to shared memory
			outfile.write ((char*)shmBase,size);
    	}
    	segmentName.clear();
    	segmentName = (sharedMemPrefix + "sa");
    	{
    		std::ofstream outfile ((outputDirName + "/" +"sa.bin"),std::ofstream::binary);

			uint64_t size = shared_mem::shmSegmentToSizeMap[(sharedMemPrefix + "sa")];

			int shmFd;
			size_t dataSize = static_cast<std::size_t>(size);
	        std::cerr << "Segment name = " << segmentName << std::endl;
	        std::cerr << "Data size = " << dataSize << std::endl;
			// Size on the shared memory that was allocated to hold the vector data
			off_t shmSize = static_cast<off_t>(((dataSize/shared_mem::SHM_PAGE_SIZE) + 1 ) * shared_mem::SHM_PAGE_SIZE);
	        void *shmBase = shared_mem::initSharedMemory(segmentName, shmSize, shmFd);
	        // read content of infile to shared memory
			outfile.write ((char*)shmBase,size);
    	}
	}


}
