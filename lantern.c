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

void echo(float delay);

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

unsigned int getNextColour(unsigned int currentColour)
{
	unsigned int nextColor = 0;
	switch(currentColour)
	{
		case R:
			nextColor = G;
			break;
		case G:
			nextColor = B;
			break;
		case B:
			nextColor = R|G;
			break;
		case R|G:
			nextColor = G|B;
			break;
		case G|B:
			nextColor = B|R;
			break;
		case B|R:
			nextColor = R|G|B;
			break;
		case R|G|B:
			nextColor = R;
			break;
		case 0:
			nextColor = R;
			break;
			
		default:
			break;
	}

	return nextColor;
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

void floodleft(bool init, int initColour, bool autoSwitch)
{
	unsigned int main, children, overflow;
	main = AVAL & MAIN_MASK; //current value of main nodes
	children = AVAL & CHILD_MASK; //current value of child nodes

	if(init)
	{
		children = initColour;
	}
	else if(autoSwitch && (((children & MS_CHILD_MASK) >> 15) == (children & LS_CHILD_MASK))) // completed a full cycle
	{
		unsigned int nextColor = getNextColour(children & LS_CHILD_MASK);

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

void floodright(bool init, int initColour, bool autoSwitch)
{
	unsigned int main, children, overflow;
	main = AVAL & MAIN_MASK; //current value of main nodes
	children = AVAL & CHILD_MASK; //current value of child nodes

	if(init)
	{
		children = initColour << 15;
	}
	else if(autoSwitch && (((children & MS_CHILD_MASK) >> 15) == (children & LS_CHILD_MASK))) // completed a full cycle
	{
		unsigned int nextColor = getNextColour(children & LS_CHILD_MASK);
		children  = (children & ~MS_CHILD_MASK) | (nextColor << 15);
	}
	else
	{
		unsigned int msColor = children & MS_CHILD_MASK;
		children = (children >> 3) | msColor;
	}

	AVAL = main | children; //set the new pattern to AVAL
}

void floodlift(bool init, int initColour, bool autoSwitch, bool directionUp)
{
	unsigned int main, children, filler;
	main = AVAL & MAIN_MASK; //current value of main nodes
	children = AVAL & CHILD_MASK; //current value of child nodes

	if(init)
	{
		if(directionUp)
			main = initColour << 18;
		else
			main = initColour << 27;
	}
	else if(autoSwitch && ((main & MS_MAIN_MASK) >> 9) ==  (main & LS_MAIN_MASK)) // completed a full cycle
	{
		filler = getNextColour((main & LS_MAIN_MASK) >> 18);
		if(directionUp)
		{
			main = ((main << 3) & MAIN_MASK); //shift the main nodes to left
			main = main | (filler << 18); // restore least significant main node
		}
		else
		{
			main = ((main >> 3) & MAIN_MASK); //shift the main nodes to right
			main = main | (filler << 27); // restore most significant main node
		}
	}
	else
	{
		if(directionUp)
		{
			filler = main & LS_MAIN_MASK; //the least significant main node
			main = ((main << 3) & MAIN_MASK); //shift the main nodes to left
			main = main | filler; // restore least significant main node
		}
		else
		{
			filler = main & MS_MAIN_MASK; //the most significant main node
			main = ((main >> 3) & MAIN_MASK); //shift the main nodes to right
			main = main | filler; // restore most significant main node
		}
	}

	AVAL = main | children; //set the new pattern to AVAL
}

void flashMain(float period, int iterations)
{
	unsigned int main = AVAL & MAIN_MASK; //current value of main nodes
	unsigned int children = AVAL & CHILD_MASK; //current value of child nodes
	int i = 0;

	for(i = 0; i<iterations; ++i)
	{
		AVAL = children; //clear all main nodes
		echo(period);

		AVAL = main | children; //restore all main nodes
		echo(period);
	}
}

void flashChildren(float period, int iterations)
{
	unsigned int main = AVAL & MAIN_MASK; //current value of main nodes
	unsigned int children = AVAL & CHILD_MASK; //current value of child nodes
	int i = 0;

	for(i = 0; i<iterations; ++i)
	{
		AVAL = main; //clear all child nodes
		echo(period);

		AVAL = main | children; //restore all child nodes
		echo(period);
	}
}

void flashAll(float period, int iterations)
{
	unsigned int colour = AVAL; //current value of all nodes
	int i = 0;

	for(i = 0; i<iterations; ++i)
	{
		AVAL = 0; //clear all nodes
		echo(period);

		AVAL = colour; //restore all  nodes
		echo(period);
	}
}

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

void echo(float delay)
{
	off_t port = 0xE8000020;
	int i;

	//The following port configuration does not need to be done on evey echo. However
	//there is no harm in doing it and it takes care of spurious port reconfigurations happen
	//during system restart.
	//BEGIN port configuration
	writeport(0xe8000030, 0); //set row A to all GPIO mode
	writeport(0xE8000010, 0x0); //set all port A pins to be low
	//END port configuration


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

	if(delay < 0)
		delay = DELAY1;

	usleep(delay * 1000000);
}

void setMain(unsigned int rgb)
{
	int i;
	for(i=6; i<10; ++i)
		set(i, rgb);
}


void setChildren(unsigned int rgb)
{
	int i;
	for(i=0; i<6; ++i)
		set(i, rgb);
}

void setAll(unsigned int rgb)
{
	int i;
	for(i=0; i<10; ++i)
		set(i, rgb);
}

void flashMainColourSwitch()
{
	int i = 0;
	float timing = -1;
	for(i=0, timing=-1; i<4; ++i, timing=0.5)
	{
		setMain(R);
		flashMain(timing, 1);
		setMain(G);
		flashMain(timing, 1);
		if(i<3)
		{
			setMain(B);
			flashMain(timing, 1);
		}
	}
}

int main(int argc, char **argv)
{
	unsigned int i;

	//set row A to all GPIO mode
	writeport(0xe8000030, 0);

	//set row A to all input
	writeport(0xe8000020, 0xffffffff);

	//set all port A pins to be low
	writeport(0xE8000010, 0x0);
	

/*
	rotate(true, true);
	lift(true, true);

	echo(-1);
*/

/*
	fprintf(stderr, "Rotate Right\n");
	for(i=0; i<10; ++i)
	{
		rotate(false, true);
		echo(-1);
	}
*/
/*
	fprintf(stderr, "Rotate Left\n");
	for(i=0; i<10; )
	{
		rotate(false, false);
		echo(-1);
	}
*/

	if(argc == 1)
	{
		unsigned int colour = 0;
		unsigned int t = 0;
		unsigned int cCounter = 0;
		unsigned int mCounter = 0;
		unsigned int mPeriod = 60; //seconds
		float timing = -1;

		DELAY1 = 2; //seconds
		AVAL = 0;
		echo(-1);

		for(;;)
		{
			fprintf(stdout, "Flood Lift UP\n");
			floodlift(true, R, true, true);
			echo(-1);

			for(i=0; i<11; ++i)
			{
				floodlift(false, R, true, true);

				if(i < 3)
					echo(-1);
				else
					echo(0.5);
			}


			//fprintf(stdout, "Flash Main\n");
			//flashMain(1, 4);
			//flashMain(0.5, 4);
			
			flashMainColourSwitch();
			//fprintf(stdout, "Switching colours in the main and flashing\n");
			//for(i=0, timing=-1; i<4; ++i, timing=0.5)
			//{
			//	setMain(R);
			//	flashMain(timing, 1);
			//	setMain(G);
			//	flashMain(timing, 1);
			//	setMain(B);
			//	flashMain(timing, 1);
			//}

			fprintf(stdout, "Flood Left\n");
			for(i=0; i<60; ++i)
			{
				floodleft(false, R, true);
				echo(0.25);
			}
			echo(1.5);

			flashMainColourSwitch();
			//fprintf(stdout, "Switching colours in the main and flashing\n");
			//for(i=0, timing=-1; i<4; ++i, timing=0.5)
			//{
			//	setMain(R);
			//	flashMain(timing, 1);
			//	setMain(G);
			//	flashMain(timing, 1);
			//	if(i<4)
			//	{
			//		setMain(B);
			//		flashMain(timing, 1);
			//	}
			//}

			fprintf(stdout, "Flood Right\n");
			for(i=0; i<60; ++i)
			{
				floodright(false, R, true);
				echo(0.25);
			}
			echo(1.5);

			fprintf(stdout, "Flash All\n");
			flashAll(1, 4);
			flashAll(0.5, 4);

			fprintf(stdout, "Set All RGB\n");
			for(i=0; i<10; ++i)
			{
				setAll(R);
				echo(-1);

				setAll(G);
				echo(-1);

				setAll(B);
				echo(-1);
			}

			fprintf(stdout, "Rotate and Lift\n");
			rotate(1, true);
			lift(true, true);
			echo(-1);
			for(i=0; i<30; ++i)
			{
				rotate(5, true);
				lift(false, true);
				echo(-1);
			}

			fprintf(stdout, "Flood Right\n");
			setAll(G);
			floodright(true, B, true);
			floodlift(true, R, true, false);
			echo(-1);
			for(i=0; i<30; ++i)
			{
				floodright(false, B, true);
				floodlift(false, R, true, false);
				echo(-1);			
			}

			fprintf(stdout, "Set All RGB\n");
			for(i=0; i<5; ++i)
			{
				setAll(R);
				echo(-1);
				setAll(G);
				echo(-1);
				setAll(B);
				echo(-1);
			}

			fprintf(stdout, "\n\nRepeating Over\n\n");
			setAll(0);
			echo(-1);			
		}

	}
	else
	{
		int mode = atoi(argv[1]);
		int autoSwitch = false;
		int colour = R;

		if(argc > 2)
			colour = atoi(argv[2]);

		if(argc > 3)
			autoSwitch = atoi(argv[3]) > 0;


		bool init = true;

		for(;;)
		{
			switch(mode)
			{
				case 0: 
				fprintf(stdout, "Flood Left\n");
				floodleft(init, colour, autoSwitch);
				break;

				case 1: 
				fprintf(stdout, "Flood Left\n");
				floodleft(init, colour, autoSwitch);
				break;

				case 2: 
				fprintf(stdout, "Flood Left\n");
				floodright(init, colour, autoSwitch);
				break;

				case 3: 
				fprintf(stdout, "Flood Up\n");
				floodlift(init, colour, autoSwitch, true);
				break;

				case 4: 
				fprintf(stdout, "Flood Down\n");
				floodlift(init, colour, autoSwitch, false);
				break;

				case 5:
				fprintf(stdout, "Up\n");
				lift(init, true);
				break;

				case 6:
				fprintf(stdout, "Down\n");
				lift(init, false);
				break;

				case 7:
				fprintf(stdout, "Rotate Right with two colours\n");
				rotate(2, true);
				break;

				case 8:
				setAll(R);
				echo(-1);
				flashChildren(-1, 5);
				break;

				case 9:
				setAll(G);
				echo(-1);
				flashMain(-1, 5);
				break;

				default:
				break;
			}
			echo(-1);
			init = false;

			if(mode == 0)
				break;
		}
	}




	return 0;
}

