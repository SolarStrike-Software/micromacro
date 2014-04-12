#include "rng.h"
#include "timer.h"

#include <cstdlib>

int random(int min, int max)
{
	static int seeded = false;
	if( !seeded ) {
		srand(getNow().LowPart);
		seeded = true;
	}

	int spread = max-min;
	int value = min + (rand() % spread);

	return value;
}
