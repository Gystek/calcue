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

static double
factorial (n)
	int n;
{
    double f = 1;

    while (n > 0)
    {
        f *= n--;
    }

    return f;
}

double
pow (x, y)
	double x, y;
{
    return exp (y * log (x));
}

#define EULER (2.718281828459045235)

#ifndef NAN
# define NAN (0.0/0.0)
#endif

#ifndef _LOG_PRECISION
# define _LOG_PRECISION (100)
#endif

/* Calculates the natural logarithm of x using Taylor series development */
double
log (x)
	double x;
{
    int k;
    double dl, y;
    int clipping = 0;

    if (x == 0)
        return NAN;

    while (x > 1.0)
    {
        x /= EULER;
        clipping += 1;
    }

    while (x < 0.25)
    {
        x *= EULER;
        clipping -= 1;
    }

    /* Taylor development is for log(1 + x) */
    x -= 1.0;

    /* log(1 + x) = x - x^2 / 2 + x^3 / 3 + ... */
    y = x;
    dl = 0.0;
    for (k = 1; k < _LOG_PRECISION; k++)
    {
        dl += y / k;
        y *= -x;
    }

    return dl + clipping;
}

#ifndef _EXP_PRECISION
# define _EXP_PRECISION (20)
#endif

/* Calculates the exponential of x using Taylor series development */
double
exp (x)
	double x;
{
    int k;
    double dl, y;

    if (x == 0.0)
        return 1.0;

    if (x == 1.0)
        return EULER;

    /* exp(x) = 1 + x + x^2 / 2! + ... */
    y = 1;
    dl = 0.0;
    for (k = 0; k < _EXP_PRECISION; k++)
    {
        dl += y / factorial (k);
        y *= x;
    }

    return dl;
}

static inline double
max (x, y)
	double x, y;
{
    if (x > y)
        return x;
    return y;
}

static inline double
min (x, y)
	double x, y;
{
    if (x < y)
        return x;
    return y;
}

static inline double
absd (x)
	double x;
{
    return max (x, -x);
}

/* Calculates the square root of x using Quake III's rsqrt
 * (although adapted for double precision numbers)
 * */
double
sqrt (x)
	double x;
{
    int64_t i;
    double x2, y;

    x2 = x * 0.5;
    y  = x;

    i  = *(int64_t *)&y;
    i  = 0x5fe6eb50c7b537a9 - (i >> 1);

    y  = *(double *)&i;
    y  = y * (1.5 - (x2 * y * y));
    y  = y * (1.5 - (x2 * y * y));

    return 1 / y;
}
