/*
 * ====================================================
 * Copyright (C) 2004 by Sun Microsystems, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 *
 * Math standard library routines taken from fdlibm:
 *
 *   http://www.netlib.org/fdlibm/
 *
 * Modified for use with BITFLIPS valgrind tool.
 *
 */
#ifndef __BITFLIPS_MATH_H
#define __BITFLIPS_MATH_H

#if defined(__i386__) || defined(__i486__) || \
    defined(__intel__) || defined(__x86__) || defined(__x86_64__) || \
    defined(__i86pc__) || defined(__alpha) || defined(__osf__)
#define __LITTLE_ENDIAN
#endif

#ifdef __LITTLE_ENDIAN
#define __HI(x) *(1+(int*)&x)
#define __LO(x) *(int*)&x
#define __HIp(x) *(1+(int*)x)
#define __LOp(x) *(int*)x
#else
#define __HI(x) *(int*)&x
#define __LO(x) *(1+(int*)&x)
#define __HIp(x) *(int*)x
#define __LOp(x) *(1+(int*)x)
#endif

#define M_PI 3.14159265358979323846264338327950288

/*
 * log(x) returns the logarithm of x.
 */
double log(double x);

/* 
 * sqrt(x) returns correctly rounded sqrt.
 */
double sqrt(double x);

/*
 * exp(x) returns the exponential of x.
 */
double exp(double x);

/*
 * floor(x) returns x rounded toward -inf to integral value.
 */
double floor(double x);

/*
 * fabs(x) returns the absolute value of x.
 */
double fabs(double x);

#endif  /* __BITFLIPS_MATH_H */
