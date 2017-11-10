/**
 * Simple program demonstrating shared memory in POSIX systems.
 *
 * This is the consumer process
 *
 * Figure 3.18
 *
 * @author Gagne, Galvin, Silberschatz
 * Operating System Concepts - Ninth Edition
 * Copyright John Wiley & Sons - 2013
 *
 * modifications by dheller@cse.psu.edu, 31 Jan. 2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <ext/stdio_filebuf.h>
#include <iostream>
#include <fstream>

void display(char *prog, char *bytes, int n);

int main(void)
{
  const char *name = "/shm-example";	// file name
  const int SIZE = 4096;		// file size

  int shm_fd;		// file descriptor, from shm_open()
  char *shm_base;	// base address, from mmap()

  /* open the shared memory segment as if it was a file */
  shm_fd = shm_open(name, O_RDONLY, 0666);
  if (shm_fd == -1) {
    printf("cons: Shared memory failed: %s\n", strerror(errno));
    exit(1);
  }

  /* configure the size of the shared memory segment */
  ftruncate(shm_fd, SIZE);

  /* @Abdullah, trying to make the shm_fd be used as a ofstream*/
  // __gnu_cxx::stdio_filebuf<char> shmFileBuf(shm_fd, std::ios::in);
  // std::istream shmStream(&shmFileBuf);

  // std::string line;
  // getline(shmStream, line);
  // std::cout << "line: " << line << std::endl;
  
  /********************************************/
  /* map the shared memory segment to the address space of the process */
  shm_base = (char *)mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
  if (shm_base == MAP_FAILED) {
    printf("cons: Map failed: %s\n", strerror(errno));
    // close and unlink?
    exit(1);
  }

  FILE *stream;

  stream = fmemopen(shm_base, SIZE-1, "r");
  if (stream == NULL)
  {
      printf("prod: Error opening the share memory: %s\n", strerror(errno));
  }

  int streamBuf = fileno(stream);
  __gnu_cxx::stdio_filebuf<char> shmFileBuf(streamBuf, std::ios::in);
  std::istream shmStream(&shmFileBuf);

  std::string line;
  getline(shmStream, line);
  std::cout << "line: " << line << std::endl;

  // char *line = (char *)malloc(sizeof(char) * 32);
  // size_t len = 32;
  // getline(&line, &len, stream);
  // std::cout << "line: " << line << std::endl;

  //  read from the mapped shared memory segment 
  display("cons", shm_base, 64);	// first as bytes, then as a string
  printf("%s", shm_base);

  /* remove the mapped shared memory segment from the address space of the process */
  if (munmap(shm_base, SIZE) == -1) {
    printf("cons: Unmap failed: %s\n", strerror(errno));
    exit(1);
  }

  /* close the shared memory segment as if it was a file */
  if (close(shm_fd) == -1) {
    printf("cons: Close failed: %s\n", strerror(errno));
    exit(1);
  }

  /* remove the shared memory segment from the file system */
  if (shm_unlink(name) == -1) {
    printf("cons: Error removing %s: %s\n", name, strerror(errno));
    exit(1);
  }

  return 0;
}

void display(char *prog, char *bytes, int n)
{
  printf("display: %s\n", prog);
  for (int i = 0; i < n; i++)
    { printf("%02x%c", bytes[i], ((i+1)%16) ? ' ' : '\n'); }
  printf("\n");
}

