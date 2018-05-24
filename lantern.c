#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

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
float DELAY1 = 0.5; //seconds

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

void rotate(int seed, bool direction)
{
	unsigned int main, children, overflow;
	main = AVAL & MAIN_MASK; //current value of main nodes
	children = AVAL & CHILD_MASK; //current value of child nodes

	switch(seed)
	{
		case 1:
			children = R | G << 3 | B << 6 | R << 9 | G << 12 | B << 15;
			//children = R | G << 3 | B << 6 ;//| R << 9 | G << 12 | B << 15;
			break;
		case 2:
			children = (R|G) | (G|B) << 3 | (B|R) << 6 | (R|G) << 9 | (G|B) << 12 | (B|R) << 15;
			break;
		case 3:
			children = (R|B) | (G|R) << 3 | (B|G) << 6 | (R|B) << 9 | (G|R) << 12 | (B|G) << 15;
			break;
		default:
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
			break;
	}

	AVAL = main | children; //set the new pattern to AVAL
}

void floodleft(bool init, bool autoSwitch)
{
	unsigned int main, children, overflow;
	main = AVAL & MAIN_MASK; //current value of main nodes
	children = AVAL & CHILD_MASK; //current value of child nodes

	if(init)
	{
		children = R;
	}
	else if(autoSwitch && (((children & MS_CHILD_MASK) >> 15) == (children & LS_CHILD_MASK))) // completed a full cycle
	{
		unsigned int nextColor = 0;
		switch(children & LS_CHILD_MASK)
		{
			case R:
				nextColor = G;
				break;
			case G:
				nextColor = B;
				break;

			case B:
			default:
				nextColor = R;
				break;
		}

		children  = (children & ~LS_CHILD_MASK) | nextColor;
	}
	else
	{
		unsigned int lsColor = children & LS_CHILD_MASK;
		children = (children << 3) | lsColor;
		children = children & CHILD_MASK;
	}

	AVAL = main | children; //set the new pattern to AVAL
}

void floodright(bool init, bool autoSwitch)
{
	unsigned int main, children, overflow;
	main = AVAL & MAIN_MASK; //current value of main nodes
	children = AVAL & CHILD_MASK; //current value of child nodes

	if(init)
	{
		children = R << 15;
	}
	else if(autoSwitch && (((children & MS_CHILD_MASK) >> 15) == (children & LS_CHILD_MASK))) // completed a full cycle
	{
		unsigned int nextColor = 0;
		switch(children & LS_CHILD_MASK)
		{
			case R:
				nextColor = G;
				break;
			case G:
				nextColor = B;
				break;

			case B:
			default:
				nextColor = R;
				break;
		}

		children  = (children & ~MS_CHILD_MASK) | (nextColor << 15);
	}
	else
	{
		unsigned int msColor = children & MS_CHILD_MASK;
		children = (children >> 3) | msColor;
	}

	AVAL = main | children; //set the new pattern to AVAL
}
/*
void floodup(int initColour, bool autoSwitch)
{
	unsigned int main, children, overflow;
	main = AVAL & MAIN_MASK; //current value of main nodes
	children = AVAL & CHILD_MASK; //current value of child nodes

	if(initColour >= 0)
	{
		children = initColour;
	}
	else if(autoSwitch && (((children & MS_CHILD_MASK) >> 15) == (children & LS_CHILD_MASK))) // completed a full cycle
	{
		unsigned int nextColor = 0;
		switch(children & LS_CHILD_MASK)
		{
			case R:
				nextColor = G;
				break;
			case G:
				nextColor = B;
				break;

			case B:
			default:
				nextColor = R;
				break;
		}

		children  = (children & ~LS_CHILD_MASK) | nextColor;
	}
	else
	{
		unsigned int lsColor = children & LS_CHILD_MASK;
		children = (children << 3) | lsColor;
		children = children & CHILD_MASK;
	}

	AVAL = main | children; //set the new pattern to AVAL
}
*/
void lift(bool init, bool directionUp)
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
		if(directionUp)
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
	off_t port = 0xE8000020;
	int i;

	writeport(port, AVAL);
	unsigned int node;

	fprintf(stdout, "Children: ");
	for(i=0; i<10; ++i)
	{
		node = get(i);
		fprintf(stdout, "%c", (node & R) ? 'R' : '.');
		fprintf(stdout, "%c", (node & G) ? 'G' : '.');
		fprintf(stdout, "%c", (node & B) ? 'B' : '.');

		fprintf(stdout, " | ");
		if(i == 5)
			fprintf(stdout, "   Main: ");
	}
	fprintf(stdout, "\n");

	usleep(DELAY1 * 1000000);
}

int main(int argc, char **argv)
{
	unsigned int i;

/*
	rotate(true, true);
	lift(true, true);

	echo();
*/

/*
	fprintf(stderr, "Rotate Right\n");
	for(i=0; i<10; ++i)
	{
		rotate(false, true);
		echo();
	}
*/
/*
	fprintf(stderr, "Rotate Left\n");
	for(i=0; i<10; )
	{
		rotate(false, false);
		echo();
	}
*/

	if(argc == 1)
	{
		//DELAY1 = 0.2; // seconds
		for(i = 0; ; ++i)
		{
			fprintf(stderr, "Flood Left\n");
			floodleft(true, false);
			echo();
			for(i=0; i<10; )
			{
				floodleft(false, false);
				echo();
			}
		}

	}
	else
	{
		int mode = atoi(argv[1]);

		if(argc > 2)
			DELAY1 = atof(argv[2]);

		bool init = true;

		for(;;)
		{
			switch(mode)
			{
				case 1: 
				fprintf(stdout, "Left\n");
				floodleft(init, false);
				echo();
				break;

				case 2:
				fprintf(stdout, "Right\n");
				floodright(init, false);
				echo();
				break;

				case 3:
				fprintf(stdout, "Up\n");
				lift(init, true);
				echo();
				break;

				case 4:
				fprintf(stdout, "Down\n");
				lift(init, false);
				echo();
				break;

				case 5:
				fprintf(stdout, "Rotate Right with two colours\n");
				rotate(2, true);
				break;

				default:
				break;
			}

			init = false;
		}
	}




	return 0;
}

