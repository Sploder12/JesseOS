#include "math.h"

int pow(int base, int exp)
{
	if (exp == 0)
		return 1;
	
	if (exp % 2 == 0)
		return pow(base, exp * 0.5) * pow(base, exp * 0.5);
		
	return base * pow(base, exp *0.5) * pow(base, exp * 0.5);
}
