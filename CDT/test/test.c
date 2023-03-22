/*
 * test.c
 *
 *  Created on: 12 de mar. de 2023
 *      Author: Alexandre
 */

#include "test.h"

#ifdef TEST_EXPAND

int main()
{
	initAutomaton();
	//
	for(int i = 0; i < 1; i++)
	{
		test_expand();
	}
	fflush(stdout);
	return 0;
}

#endif

#ifdef TEST_TREE

int xxxmain()
{
	fflush(stdout);
	return 0;
}

#endif
