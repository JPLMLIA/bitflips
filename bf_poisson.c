/*
 *  ## NumPy
 *  
 *  Copyright (c) 2005-2017, NumPy Developers.
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  
 *  * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  
 *  * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *  
 *  * Neither the name of the NumPy Developers nor the names of any
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  Poisson random variable implementation take from NumPy:
 *
 *  https://github.com/numpy/numpy/blob/master/numpy/random/src/distributions/distributions.c
 *
 */
#include "bf_math.h"
#include "bf_poisson.h"

/*
 * log-gamma function to support some of these distributions. The algorithm
 * comes from SPECFUN by Shanjie Zhang and Jianming Jin and their book
 * "Computation of Special Functions", 1996, John Wiley & Sons, Inc.
 *
 * If random_loggam(k+1) is being used to compute log(k!) for an integer k,
 * consider using logfactorial(k) instead.
 */
static double random_loggam(double x) {
    double x0, x2, xp, gl, gl0;
    int k, n;

    static double a[10] = {8.333333333333333e-02, -2.777777777777778e-03,
                           7.936507936507937e-04, -5.952380952380952e-04,
                           8.417508417508418e-04, -1.917526917526918e-03,
                           6.410256410256410e-03, -2.955065359477124e-02,
                           1.796443723688307e-01, -1.39243221690590e+00};
    x0 = x;
    n = 0;
    if ((x == 1.0) || (x == 2.0)) {
        return 0.0;
    } else if (x <= 7.0) {
        n = (int)(7 - x);
        x0 = x + n;
    }
    x2 = 1.0 / (x0 * x0);
    xp = 2 * M_PI;
    gl0 = a[9];
    for (k = 8; k >= 0; k--) {
        gl0 *= x2;
        gl0 += a[k];
    }
    gl = gl0 / x0 + 0.5 * log(xp) + (x0 - 0.5) * log(x0) - x0;
    if (x <= 7.0) {
        for (k = 1; k <= n; k++) {
            gl -= log(x0 - 1.0);
            x0 -= 1.0;
        }
    }
    return gl;
}

static int random_poisson_mult(double lambda, double (*next_double)(void)) {
    int X;
    double prod, U, enlam;

    enlam = exp(-lambda);
    X = 0;
    prod = 1.0;
    while (1) {
        U = next_double();
        prod *= U;
        if (prod > enlam) {
            X += 1;
        } else {
            return X;
        }
    }
}

/*
 * The transformed rejection method for generating Poisson random variables
 * W. Hoermann
 * Insurance: Mathematics and Economics 12, 39-45 (1993)
 */
static int random_poisson_ptrs(double lambda, double (*next_double)(void)) {
    int k;
    double U, V, slam, loglam, a, b, invalpha, vr, us;

    slam = sqrt(lambda);
    loglam = log(lambda);
    b = 0.931 + 2.53 * slam;
    a = -0.059 + 0.02483 * b;
    invalpha = 1.1239 + 1.1328 / (b - 3.4);
    vr = 0.9277 - 3.6224 / (b - 2);

    while (1) {
        U = next_double() - 0.5;
        V = next_double();
        us = 0.5 - fabs(U);
        k = (int)floor((2 * a / us + b) * U + lambda + 0.43);
        if ((us >= 0.07) && (V <= vr)) {
            return k;
        }
        if ((k < 0) || ((us < 0.013) && (V > us))) {
            continue;
        }
        /* log(V) == log(0.0) ok here */
        /* if U==0.0 so that us==0.0, log is ok since always returns */
        if ((log(V) + log(invalpha) - log(a / (us * us) + b)) <=
            (-lambda + k * loglam - random_loggam(k + 1))) {
            return k;
        }
    }
}

/**
 *  Return a Poisson-distributed random variable with rate parameter lambda and
 *  source of double values drawn uniformly at random from [0, 1].
 */
int random_poisson(double lambda, double (*next_double)(void)) {
    if (lambda >= 10) {
        return random_poisson_ptrs(lambda, next_double);
    } else if (lambda == 0) {
        return 0;
    } else {
        return random_poisson_mult(lambda, next_double);
    }
}
