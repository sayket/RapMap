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




namespace shared_mem
{
	const int defaultFileMode = 0666;
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
	void * initSharedMemory(std::string name, int oflag, mode_t mode, off_t size, int *fd);

	int deinitializeSharedMemory();

	// @brief unilinks a shared memory segment with name 'name'
	// if no other process points to the file anymore,
	// this functions removes the shared memory segment from the memory
	// it uses shm_unlink underneath the hood
	// @param 'name' name of the segment to unlink
	int removeSharedMemory(std::string name);
	// int shm_unlink(const char *name);

	// @brief returns the output stream pointed by the name
	// @param name is the name of the shared memory segment
	// @return returns a output stream, this can be used by cereal
	// to write the output in the shared memory
	// we use this output stream to write archive data to shared memory
	// we will read the archive data in the similar manner from the shared mem
	std::ostream& getOutputStream(std::string name);

	// @brief shm_base get output stream from a shared memory address
	// @param shared meory address
	std::ostream& getOutputStream(void *shm_base);

	// @brief returns the intput stream pointed by the name
	// @param name is the name of the shared memory segment to read from
	// @return returns a intput stream, this can be used by cereal
	// to read the input from the shared memory segment
	// we use this input stream to read archive data from shared memory
	std::istream& getInputStream(std::string name);

	// @brief shm_base get input stream from a shared memory address
	// @param shared meory address
	std::ostream& getOutputStream(void *shm_base);

}

#endif //__RAPMAP_SHARED_MEM_HPP__
