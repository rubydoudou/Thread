/* Programming Assignment 4: Exercise B
 */

#include "aux.h"
#include "umix.h"
#include "mycode4.h"

#define NUMYIELDS	5

static int square;			// global variable, shared by threads

void Main()
{
	int i, t;
	void printSquares();

	MyInitThreads();		// initialize, must be called first

	MyCreateThread(printSquares, 0);

	for (i = 0; i < NUMYIELDS; i++) {
		MyYieldThread(1);
		Printf("T0: square = %d\n", square);
	}

	MyExitThread();
}

void printSquares(int t)
	// t: thread to yield to
{
	int i;

	for (i = 0; i < NUMYIELDS; i++) {
		square = i * i;
		Printf("T1: %d squared = %d\n", i, square);
		MyYieldThread(0);
	}
}
