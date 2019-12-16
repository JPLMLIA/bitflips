/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
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

#include "bf_math.h"

#define HUGE 1.0e300
#define TINY 1.0e-300
#define ONE 1.0
#define ZERO 0.0

static const double
    halF[2] = {0.5,-0.5,},
    twom1000 = 9.33263618503218878990e-302,     /* 2**-1000=0x01700000,0*/
    o_threshold =  7.09782712893383973096e+02,  /* 0x40862E42, 0xFEFA39EF */
    u_threshold = -7.45133219101941108420e+02,  /* 0xc0874910, 0xD52D3051 */
    ln2HI[2]   = {
         6.93147180369123816490e-01,  /* 0x3fe62e42, 0xfee00000 */
        -6.93147180369123816490e-01, /* 0xbfe62e42, 0xfee00000 */
    },
    ln2LO[2] = {
         1.90821492927058770002e-10,  /* 0x3dea39ef, 0x35793c76 */
        -1.90821492927058770002e-10, /* 0xbdea39ef, 0x35793c76 */
    },
    invln2 = 1.44269504088896338700e+00, /* 0x3ff71547, 0x652b82fe */
    P1 =  1.66666666666666019037e-01, /* 0x3FC55555, 0x5555553E */
    P2 = -2.77777777770155933842e-03, /* 0xBF66C16C, 0x16BEBD93 */
    P3 =  6.61375632143793436117e-05, /* 0x3F11566A, 0xAF25DE2C */
    P4 = -1.65339022054652515390e-06, /* 0xBEBBBD41, 0xC5D26BF1 */
    P5 =  4.13813679705723846039e-08, /* 0x3E663769, 0x72BEA4D0 */
    two54 =  1.80143985094819840000e+16,  /* 43500000 00000000 */
    Lg1 = 6.666666666666735130e-01,  /* 3FE55555 55555593 */
    Lg2 = 3.999999999940941908e-01,  /* 3FD99999 9997FA04 */
    Lg3 = 2.857142874366239149e-01,  /* 3FD24924 94229359 */
    Lg4 = 2.222219843214978396e-01,  /* 3FCC71C5 1D8E78AF */
    Lg5 = 1.818357216161805012e-01,  /* 3FC74664 96CB03DE */
    Lg6 = 1.531383769920937332e-01,  /* 3FC39A09 D078C69F */
    Lg7 = 1.479819860511658591e-01;  /* 3FC2F112 DF3E5244 */

/*
 * log(x) returns the logrithm of x.
 *
 * Method :
 *   1. Argument Reduction: find k and f such that
 *              x = 2^k * (1+f),
 *       where  sqrt(2)/2 < 1+f < sqrt(2) .
 *
 *   2. Approximation of log(1+f).
 *    Let s = f/(2+f) ; based on log(1+f) = log(1+s) - log(1-s)
 *          = 2s + 2/3 s**3 + 2/5 s**5 + .....,
 *          = 2s + s*R
 *    We use a special Reme algorithm on [0,0.1716] to generate
 *    a polynomial of degree 14 to approximate R The maximum error
 *    of this polynomial approximation is bounded by 2**-58.45. In
 *    other words,
 *                    2      4      6      8      10      12      14
 *        R(z) ~ Lg1*s +Lg2*s +Lg3*s +Lg4*s +Lg5*s  +Lg6*s  +Lg7*s
 *      (the values of Lg1 to Lg7 are listed in the program)
 *    and
 *        |      2          14          |     -58.45
 *        | Lg1*s +...+Lg7*s    -  R(z) | <= 2
 *        |                             |
 *    Note that 2s = f - s*f = f - hfsq + s*hfsq, where hfsq = f*f/2.
 *    In order to guarantee error in log below 1ulp, we compute log
 *    by
 *        log(1+f) = f - s*(f - R)    (if f is not too large)
 *        log(1+f) = f - (hfsq - s*(hfsq+R)).    (better accuracy)
 *
 *    3. Finally,  log(x) = k*ln2 + log(1+f).
 *                = k*ln2_hi+(f-(hfsq-(s*(hfsq+R)+k*ln2_lo)))
 *       Here ln2 is split into two floating point number:
 *            ln2_hi + ln2_lo,
 *       where n*ln2_hi is always exact for |n| < 2000.
 *
 * Special cases:
 *    log(x) is NaN with signal if x < 0 (including -INF) ;
 *    log(+INF) is +INF; log(0) is -INF with signal;
 *    log(NaN) is that NaN with no signal.
 *
 * Accuracy:
 *    according to an error analysis, the error is always less than
 *    1 ulp (unit in the last place).
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following
 * constants. The decimal values may be used, provided that the
 * compiler will convert from decimal to binary accurately enough
 * to produce the hexadecimal values shown.
 */
double log(double x) {
    double hfsq,f,s,z,R,w,t1,t2,dk;
    int k,hx,i,j;
    unsigned lx;

    hx = __HI(x);        /* high word of x */
    lx = __LO(x);        /* low  word of x */

    k=0;
    if (hx < 0x00100000) {            /* x < 2**-1022  */
        if (((hx&0x7fffffff)|lx)==0) {
            return -two54/ZERO;        /* log(+-0)=-inf */
        }
        if (hx<0) {
            return (x-x)/ZERO;    /* log(-#) = NaN */
        }
        k -= 54; x *= two54; /* subnormal number, scale up x */
        hx = __HI(x);        /* high word of x */
    }
    if (hx >= 0x7ff00000) {
        return x+x;
    }
    k += (hx>>20)-1023;
    hx &= 0x000fffff;
    i = (hx+0x95f64)&0x100000;
    __HI(x) = hx|(i^0x3ff00000);    /* normalize x or x/2 */
    k += (i>>20);
    f = x-1.0;
    if((0x000fffff&(2+hx))<3) {    /* |f| < 2**-20 */
        if(f==ZERO) {
            if(k==0) {
                return ZERO;
            } else {
                dk=(double)k;
                return dk*ln2HI[0]+dk*ln2LO[0];
            }
        }
        R = f*f*(0.5-0.33333333333333333*f);
        if(k==0) {
            return f-R;
        } else {
            dk=(double)k;
            return dk*ln2HI[0]-((R-dk*ln2LO[0])-f);
        }
    }
    s = f/(2.0+f);
    dk = (double)k;
    z = s*s;
    i = hx-0x6147a;
    w = z*z;
    j = 0x6b851-hx;
    t1= w*(Lg2+w*(Lg4+w*Lg6));
    t2= z*(Lg1+w*(Lg3+w*(Lg5+w*Lg7)));
    i |= j;
    R = t2+t1;
    if(i>0) {
        hfsq=0.5*f*f;
        if(k==0) {
            return f-(hfsq-s*(hfsq+R));
        } else {
            return dk*ln2HI[0]-((hfsq-(s*(hfsq+R)+dk*ln2LO[0]))-f);
        }
    } else {
        if(k==0) {
            return f-s*(f-R);
        } else {
            return dk*ln2HI[0]-((s*(f-R)-dk*ln2LO[0])-f);
        }
    }
}

/*
 * sqrt(x) returns correctly rounded sqrt.
 *           ------------------------------------------
 *           |  Use the hardware sqrt if you have one |
 *           ------------------------------------------
 * Method:
 *   Bit by bit method using integer arithmetic. (Slow, but portable)
 *   1. Normalization
 *    Scale x to y in [1,4) with even powers of 2:
 *    find an integer k such that  1 <= (y=x*2^(2k)) < 4, then
 *        sqrt(x) = 2^k * sqrt(y)
 *   2. Bit by bit computation
 *    Let q  = sqrt(y) truncated to i bit after binary point (q = 1),
 *         i                             0
 *                                 i+1         2
 *        s  = 2*q , and    y  =  2   * ( y - q  ).        (1)
 *         i      i          i                 i
 *
 *    To compute q    from q , one checks whether
 *                i+1       i
 *
 *                  -(i+1) 2
 *            (q + 2      ) <= y.            (2)
 *              i
 *                                                          -(i+1)
 *    If (2) is false, then q   = q ; otherwise q   = q  + 2      .
 *                           i+1   i             i+1   i
 *
 *    With some algebric manipulation, it is not difficult to see
 *    that (2) is equivalent to
 *                              -(i+1)
 *            s  +  2       <= y            (3)
 *             i                i
 *
 *    The advantage of (3) is that s  and y  can be computed by
 *                                  i      i
 *    the following recurrence formula:
 *        if (3) is false
 *
 *        s     =  s  ,    y    = y   ;            (4)
 *         i+1      i       i+1    i
 *
 *        otherwise,
 *                        -i                      -(i+1)
 *        s      =  s  + 2  ,  y    = y  -  s  - 2          (5)
 *          i+1      i          i+1    i     i
 *
 *    One may easily use induction to prove (4) and (5).
 *    Note. Since the left hand side of (3) contain only i+2 bits,
 *          it does not necessary to do a full (53-bit) comparison
 *          in (3).
 *   3. Final rounding
 *    After generating the 53 bits result, we compute one more bit.
 *    Together with the remainder, we can decide whether the
 *    result is exact, bigger than 1/2ulp, or less than 1/2ulp
 *    (it will never equal to 1/2ulp).
 *    The rounding mode can be detected by checking whether
 *    huge + tiny is equal to huge, and whether huge - tiny is
 *    equal to huge for some floating point number "huge" and "tiny".
 *
 * Special cases:
 *    sqrt(+-0) = +-0     ... exact
 *    sqrt(inf) = inf
 *    sqrt(-ve) = NaN        ... with invalid signal
 *    sqrt(NaN) = NaN        ... with invalid signal for signaling NaN
 *
 * Other methods : see the appended file at the end of the program below.
 *---------------
 */
double sqrt(double x) {
#if (defined(__x86_64__) || defined(__x86__))
    // Use hardware implementation for x86 processors
    double sqrt_val;
    __asm__ ("sqrtsd %1, %0" : "=x" (sqrt_val) : "x" (x));
    return sqrt_val;
#else
    double z;
    int sign = (int)0x80000000;
    unsigned r,t1,s1,ix1,q1;
    int ix0,s0,q,m,t,i;

    ix0 = __HI(x);            /* high word of x */
    ix1 = __LO(x);        /* low word of x */

    /* take care of Inf and NaN */
    if((ix0&0x7ff00000)==0x7ff00000) {
        return x*x+x;        /* sqrt(NaN)=NaN, sqrt(+inf)=+inf
                       sqrt(-inf)=sNaN */
    }
    /* take care of zero */
    if(ix0<=0) {
        if(((ix0&(~sign))|ix1)==0) {
            return x;/* sqrt(+-0) = +-0 */
        } else if(ix0<0) {
            return (x-x)/(x-x);        /* sqrt(-ve) = sNaN */
        }
    }
    /* normalize x */
    m = (ix0>>20);
    if(m==0) {                /* subnormal x */
        while(ix0==0) {
            m -= 21;
            ix0 |= (ix1>>11);
            ix1 <<= 21;
        }
        for(i=0;(ix0&0x00100000)==0;i++) {
            ix0<<=1;
        }
        m -= i-1;
        ix0 |= (ix1>>(32-i));
        ix1 <<= i;
    }
    m -= 1023;    /* unbias exponent */
    ix0 = (ix0&0x000fffff)|0x00100000;
    if(m&1){    /* odd m, double x to make it even */
        ix0 += ix0 + ((ix1&sign)>>31);
        ix1 += ix1;
    }
    m >>= 1;    /* m = [m/2] */

    /* generate sqrt(x) bit by bit */
    ix0 += ix0 + ((ix1&sign)>>31);
    ix1 += ix1;
    q = q1 = s0 = s1 = 0;    /* [q,q1] = sqrt(x) */
    r = 0x00200000;        /* r = moving bit from right to left */

    while(r!=0) {
        t = s0+r;
        if(t<=ix0) {
            s0   = t+r;
            ix0 -= t;
            q   += r;
        }
        ix0 += ix0 + ((ix1&sign)>>31);
        ix1 += ix1;
        r>>=1;
    }

    r = sign;
    while(r!=0) {
        t1 = s1+r;
        t  = s0;
        if((t<ix0)||((t==ix0)&&(t1<=ix1))) {
            s1  = t1+r;
            if(((t1&sign)==sign)&&(s1&sign)==0) {
                s0 += 1;
            }
            ix0 -= t;
            if (ix1 < t1) {
                ix0 -= 1;
            }
            ix1 -= t1;
            q1  += r;
        }
        ix0 += ix0 + ((ix1&sign)>>31);
        ix1 += ix1;
        r>>=1;
    }

    /* use floating add to find out rounding direction */
    if((ix0|ix1)!=0) {
        z = ONE-TINY; /* trigger inexact flag */
        if (z>=ONE) {
            z = ONE+TINY;
            if (q1==(unsigned)0xffffffff) {
                q1=0; q += 1;
            }
        else if (z>ONE) {
            if (q1==(unsigned)0xfffffffe) {
                q+=1;
            }
            q1+=2;
        } else
            q1 += (q1&1);
        }
    }
    ix0 = (q>>1)+0x3fe00000;
    ix1 =  q1>>1;
    if ((q&1)==1) {
        ix1 |= sign;
    }
    ix0 += (m <<20);
    __HI(z) = ix0;
    __LO(z) = ix1;
    return z;
#endif
}

/*
 * exp(x) returns the exponential of x.
 *
 * Method
 *   1. Argument reduction:
 *      Reduce x to an r so that |r| <= 0.5*ln2 ~ 0.34658.
 *    Given x, find r and integer k such that
 *
 *               x = k*ln2 + r,  |r| <= 0.5*ln2.
 *
 *      Here r will be represented as r = hi-lo for better
 *    accuracy.
 *
 *   2. Approximation of exp(r) by a special rational function on
 *    the interval [0,0.34658]:
 *    Write
 *        R(r**2) = r*(exp(r)+1)/(exp(r)-1) = 2 + r*r/6 - r**4/360 + ...
 *      We use a special Remes algorithm on [0,0.34658] to generate
 *     a polynomial of degree 5 to approximate R. The maximum error
 *    of this polynomial approximation is bounded by 2**-59. In
 *    other words,
 *        R(z) ~ 2.0 + P1*z + P2*z**2 + P3*z**3 + P4*z**4 + P5*z**5
 *      (where z=r*r, and the values of P1 to P5 are listed below)
 *    and
 *        |                  5          |     -59
 *        | 2.0+P1*z+...+P5*z   -  R(z) | <= 2
 *        |                             |
 *    The computation of exp(r) thus becomes
 *                       2*r
 *        exp(r) = 1 + -------
 *                      R - r
 *                           r*R1(r)
 *               = 1 + r + ----------- (for better accuracy)
 *                          2 - R1(r)
 *    where
 *                         2       4             10
 *        R1(r) = r - (P1*r  + P2*r  + ... + P5*r   ).
 *
 *   3. Scale back to obtain exp(x):
 *    From step 1, we have
 *       exp(x) = 2^k * exp(r)
 *
 * Special cases:
 *    exp(INF) is INF, exp(NaN) is NaN;
 *    exp(-INF) is 0, and
 *    for finite argument, only exp(0)=1 is exact.
 *
 * Accuracy:
 *    according to an error analysis, the error is always less than
 *    1 ulp (unit in the last place).
 *
 * Misc. info.
 *    For IEEE double
 *        if x >  7.09782712893383973096e+02 then exp(x) overflow
 *        if x < -7.45133219101941108420e+02 then exp(x) underflow
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following
 * constants. The decimal values may be used, provided that the
 * compiler will convert from decimal to binary accurately enough
 * to produce the hexadecimal values shown.
 */
double exp(double x) {
    double y, hi = 0, lo = 0, c, t;
    int k = 0, xsb;
    unsigned hx;

    hx  = __HI(x);    /* high word of x */
    xsb = (hx>>31)&1;        /* sign bit of x */
    hx &= 0x7fffffff;        /* high word of |x| */

    /* filter out non-finite argument */
    if(hx >= 0x40862E42) {            /* if |x|>=709.78... */
        if(hx>=0x7ff00000) {
            if(((hx&0xfffff)|__LO(x))!=0) {
                 return x+x; /* NaN */
            }
            else {
                return (xsb==0)? x:0.0; /* exp(+-inf)={inf,0} */
            }
        }
        if(x > o_threshold) { return HUGE*HUGE; } /* overflow */
        if(x < u_threshold) { return twom1000*twom1000; } /* underflow */
    }

    /* argument reduction */
    if(hx > 0x3fd62e42) {        /* if  |x| > 0.5 ln2 */
        if(hx < 0x3FF0A2B2) {    /* and |x| < 1.5 ln2 */
            hi = x-ln2HI[xsb];
            lo = ln2LO[xsb];
            k = 1-xsb-xsb;
        } else {
            k  = (int)(invln2*x+halF[xsb]);
            t  = k;
            hi = x - t*ln2HI[0];    /* t*ln2HI is exact here */
            lo = t*ln2LO[0];
        }
        x  = hi - lo;
    }
    else if(hx < 0x3e300000)  {    /* when |x|<2**-28 */
        if(HUGE+x>ONE) { return ONE+x; } /* trigger inexact */
    }
    else {
        k = 0;
    }

    /* x is now in primary range */
    t  = x*x;
    c  = x - t*(P1+t*(P2+t*(P3+t*(P4+t*P5))));
    if(k==0) {
        return ONE-((x*c)/(c-2.0)-x);
    } else {
        y = ONE-((lo-(x*c)/(2.0-c))-hi);
    }
    if(k >= -1021) {
        __HI(y) += (k<<20);    /* add k to y's exponent */
        return y;
    } else {
        __HI(y) += ((k+1000)<<20);/* add k to y's exponent */
        return y*twom1000;
    }
}

/*
 * floor(x) returns x rounded toward -inf to integral value.
 * Method:
 *    Bit twiddling.
 * Exception:
 *    Inexact flag raised if x not equal to floor(x).
 */
double floor(double x) {
    int i0,i1,j0;
    unsigned i,j;
    i0 =  __HI(x);
    i1 =  __LO(x);
    j0 = ((i0>>20)&0x7ff)-0x3ff;
    if(j0<20) {
        if(j0<0) {     /* raise inexact if x != 0 */
            if(HUGE+x>0.0) {/* return 0*sign(x) if |x|<1 */
                if(i0>=0) {
                    i0=i1=0;
                } else if(((i0&0x7fffffff)|i1)!=0) {
                    i0=0xbff00000;i1=0;
                }
            }
        } else {
            i = (0x000fffff)>>j0;
            if(((i0&i)|i1)==0) { return x; } /* x is integral */
            if(HUGE+x>0.0) {    /* raise inexact flag */
                if(i0<0) i0 += (0x00100000)>>j0;
                i0 &= (~i); i1=0;
            }
        }
    } else if (j0>51) {
        if(j0==0x400) { return x+x; }    /* inf or NaN */
        else { return x; }        /* x is integral */
    } else {
        i = ((unsigned)(0xffffffff))>>(j0-20);
        if((i1&i)==0) { return x; }   /* x is integral */
        if(HUGE+x>0.0) {         /* raise inexact flag */
            if(i0<0) {
                if(j0==20) {
                    i0+=1;
                } else {
                    j = i1+(1<<(52-j0));
                    if(j<i1) { i0 +=1; }    /* got a carry */
                    i1=j;
                }
            }
            i1 &= (~i);
        }
    }
    __HI(x) = i0;
    __LO(x) = i1;
    return x;
}

/*
 * fabs(x) returns the absolute value of x.
 */
double fabs(double x) {
    __HI(x) &= 0x7fffffff;
    return x;
}
