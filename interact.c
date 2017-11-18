#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#define SIZE 4096
int main() {
  char *p;
  int i;
  int q;
  void *addr;
  char array[SIZE];

  for(i = 0; i < 4096; i++) {
    array[i] = 'A'; // writing 4096 'A' into an array
  }
  p = mmap(NULL, SIZE, PROT_READ | PROT_WRITE ,MAP_ANONYMOUS|MAP_PRIVATE ,-1 ,0);
  /* Created a mapped area of size 4096 because that is the size of the page - it has
   * read and write permissions enabled and is private */
  if(p == MAP_FAILED) { 
    printf("Mapping failed");
    return 1;
  } 
  mlock(p,SIZE); // Memory area gets locked so that we can use it inside the function
  memcpy(p,array,SIZE); // copied array into the page now the page is filled with "A"
  printf("pointer : %x", p);
  int fd= open("/proc/modu1", O_RDWR); //file descripter into the node of the modules
  addr=&p;   // getting the address of p
  write(fd, addr, 4); // passing address of p
  read(fd, (void *)q, 8); //using read  function
  printf("Done"); // can view the results using command dmesg
  return 0;
}
