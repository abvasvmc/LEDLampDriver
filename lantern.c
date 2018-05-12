#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>

typedef int bool;
#define true 1
#define false 0

//Mask for the least significant 30 bits (bits 0-29) of port A
#define AMASK 0x3FFFFFFF

//Mask for least significant 18 bits (bits 0-17) of port A, which represents the 6 child nodes
#define CHILD_MASK 0x3FFFF

//Mask for the least significant child node (bits 0-3)
#define LS_CHILD_MASK 0x7

//Mask for the most significant child node (bits 15-17)
#define MS_CHILD_MASK 0x38000

//Mask for the bits represents the nodes in the main body (bits 18-29)
#define MAIN_MASK 0x3FFC0000

//Mask for the least significant main node (bits 18-20)
#define LS_MAIN_MASK 0x1C0000

//Mask for the most significant main node (bits 27-29)
#define MS_MAIN_MASK 0x38000000


#define R 0x1
#define G 0x2
#define B 0x4

unsigned int AVAL;

void set(unsigned int node, unsigned int rgb)
{
	unsigned int mask, pattern, shift;
	if(node < 10)
	{
		shift = 3 * node;
		mask = ~(0x7 << shift); //mask to select all bits except the ones corresponding to the given node
		pattern = rgb << shift;
		AVAL = (AVAL & mask) | pattern;
	}
}

unsigned int get(unsigned int node)
{
	unsigned int mask, pattern, shift;
	if(node < 10)
	{
		shift = 3 * node;
		mask = (0x7 << shift); //mask to select just the bits corresponding to the given node
		pattern = AVAL & mask;
		pattern = pattern >> shift;
		return pattern;
	}
	else
		return 0;
}

void rotate(bool init, bool direction)
{
	unsigned int main, children;
	main = AVAL & MAIN_MASK; //current value of main nodes
	children = AVAL & CHILD_MASK; //current value of child nodes

	if(init)
	{
		children = R | G << 3 | B << 6 | R << 9 | G << 12 | B << 15;
	}
	else
	{
		unsigned int overflow;
		if(direction)
		{
			overflow = children & LS_CHILD_MASK; //value of the least significant child node that will overflow
			children = ((children >> 3) & CHILD_MASK) | overflow << 15; //shift the child nodes to right and insert the overflew child to the most significant node
		}
		else
		{
			overflow = children & MS_CHILD_MASK; //value of the least significant child node that will overflow
			children = ((children << 3) & CHILD_MASK) | overflow >> 15; //shift the child nodes to left and insert the overflew child to the least significant node
		}
	}

	AVAL = main | children; //set the new pattern to AVAL
}

void lift(bool init, bool direction)
{
	unsigned int main, children;
	main = AVAL & MAIN_MASK; //current value of main nodes
	children = AVAL & CHILD_MASK; //current value of child nodes

	if(init)
	{
		main = R << 18 | G << 21 | B << 24 | R << 27;
	}
	else
	{
		unsigned int carryover;
		if(direction)
		{
			main = ((main >> 3) & MAIN_MASK); //shift the main nodes to right
			carryover = main & LS_MAIN_MASK; //value of the new least significant main node
			main = main | carryover << 9; //insert the carry-over main-node to the most significant node
		}
		else
		{
			main = ((main << 3) & MAIN_MASK); //shift the main nodes to left
			carryover = main & MS_MAIN_MASK; //value of the new most significant main node
			main = main | carryover >> 9; //insert the carry-over main-node to the most significant node
		}
	}

	AVAL = main | children; //set the new pattern to AVAL
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

void echo()
{
	unsigned int node;

	fprintf(stderr, "Children: ");
	for(int i=0; i<10; ++i)
	{
		node = get(i);
		fprintf(stderr, "%c", (node & R) ? 'R' : '.');
		fprintf(stderr, "%c", (node & G) ? 'G' : '.');
		fprintf(stderr, "%c", (node & B) ? 'B' : '.');

		fprintf(stderr, " | ");
		if(i == 5)
			fprintf(stderr, "   Main: ");
	}
	fprintf(stderr, "\n");

	usleep(100000);
}

int main(int argc, char **argv)
{
	rotate(true, true);
	lift(true, true);

	echo();

	fprintf(stderr, "Rotate Right\n");
	for(int i=0; i<10; ++i)
	{
		rotate(false, true);
		echo();
	}

	fprintf(stderr, "Rotate Left\n");
	for(int i=0; i<10; ++i)
	{
		rotate(false, false);
		echo();
	}

	fprintf(stderr, "Up\n");
	for(int i=0; i<10; ++i)
	{
		lift(false, true);
		echo();
	}

	fprintf(stderr, "Down\n");
	for(int i=0; i<10; ++i)
	{
		lift(false, false);
		echo();
	}

	return 0;
}

