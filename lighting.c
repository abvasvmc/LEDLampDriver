#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>


unsigned int parseBinary(char *str) {
  unsigned int val = 0;
  
  if (*str == 'b') {
    str++;
    while (*str) {
      if (*str == '0') {
	val <<= 1;
      } else if (*str == '1') {
	val = (val << 1) + 1;
      } else {
	goto binaryError;
      }
    }
  }
  return val;
 binaryError:
  fprintf(stderr,"Unrecognized numeric value: %s\n",str);
  exit(0);
}

unsigned int parseNumber(char *str) {
  unsigned int addr = 0;

  if (!sscanf(str, "0x%x", &addr)) {
    if (!sscanf(str, "%u", &addr)) {
      addr = parseBinary(str);
    }
  }
  return addr;
}

/*
  Writes a given unsigned int value to the specified address
*/
int writeport(off_t addr, unsigned int value)
{
  off_t page;
  int fd;
  unsigned char *start;
  unsigned int *data;


  fd = open("/dev/mem", O_RDWR|O_SYNC);
  if (fd == -1) {
    perror("open(/dev/mem):");
    return -1;
  }
 
  page = addr & 0xfffff000;
  start = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, page);
  if (start == MAP_FAILED) {
    perror("mmap:");
    return -1;
  }

  data = (unsigned int *)(start + (addr & 0xfff));
  *data = value;
 
  close(fd);
  return 0;
}

int writeport2(off_t addr, unsigned int value)
{
  off_t page;
  int fd;
  unsigned char *start;
  unsigned int *data;


  fd = open("/dev/mem", O_RDWR|O_SYNC);
  if (fd == -1) {
    perror("open(/dev/mem):");
    return -1;
  }
 
  page = addr & 0xfffff000;
  start = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, page);
  if (start == MAP_FAILED) {
    perror("mmap:");
    return -1;
  }

  data = (unsigned int *)(start + (addr & 0xfff));
  *data = value;
  fprintf(stderr, "%X\n", value);

  close(fd);
  return 0;
}

void initPorts()
{
  writeport(0xE8000030, 0); // Setting port A as GPIO 
  writeport(0xE8000034, 0); // Setting port B as GPIO 

  //writeport(0xE8000020, 0x0); // Setting the entire port A as input 
  //writeport(0xE8000024, 0x0); // Setting the entire port B as input 

  writeport(0xE8000010, 0x0); // Writing 0 to the port A
  writeport(0xE8000014, 0x0); // Writing 0 to the port B
}

/*
  Features that the old peekXX/pokeXX did not have:
  1. Support for 8/16/32 bit READ/WRITE in one function
  2. Support for decimal and binary values
  3. The value return is returned (to become the status code)
 */
int main(int argc, char **argv) {

  unsigned int val;
  int i;
  off_t port;

  val = 0;
  port = argc == 1 ? 0xE8000020 : 0xE8000024;
  for( ; ; )
  {
    fprintf(stderr, "%d\n", val);
    initPorts();
    writeport(port, val);
    usleep(500000);
    val = val + 1;
    if(val == 8 )
      val = 0;
  }

  /* Configuring PC104 Connector */
  /*
  writeport(0xE8000030, 0); // Setting port A as GPIO 
  writeport(0xE8000020, 0xffffffff); // Setting the entire port A as outputs 
  usleep(100000);
  writeport(0xE8000010, 0x0);

  val = 1;
  for(i=0; i<10000000; ++i)
  {
    writeport(0xE8000020, val);
    usleep(100000);
    val = val << 2;
    if(val == 0 )
      val = 1;
  }
  */

/*
  val = 1;
  for( ; ; )
  {
    writeport(0xE8000024, val);
    usleep(100000);
    val = val << 2;
    if(val == 0 )
      val = 1;
  }
*/




  //writeport(0xE8000034, 0); /* Setting port B as GPIO */
  //writeport(0xE8000024, 0xffffffff); /* Setting the entire port B as outputs */
  //writeport(0xE8000014, 0xffffffff); /* Setting all pins in port B to be high */

  //val = 0x3FFFFFFE;
  //writeport2(0xE8000010, val);





  return 0;
}
