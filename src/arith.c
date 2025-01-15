#include <arith.h>

int32_t
powi (x, y)
	int32_t x, y;
{
    if (y < 0)
        return (x == 1) ? 1 : 0;

    if (y == 0)
        return 1;

    if (y % 2 == 0)
        return powi (x * x, y / 2);

    return x * powi (x, y - 1);
}

double
pow (x, y)
	double x, y;
{
    return x;
}

double
log (x)
	double x;
{
    return x;
}

double
exp (x)
	double x;
{
    return x;
}

double
sqrt (x)
	double x;
{
    return x;
}
