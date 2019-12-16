#ifndef __BITFLIPS_POISSON_H
#define __BITFLIPS_POISSON_H

/**
 *  Return a Poisson-distributed random variable with rate parameter lambda and
 *  source of double values drawn uniformly at random from [0, 1].
 */
int random_poisson(double lambda, double (*next_double)(void));

#endif  /* __BITFLIPS_POISSON_H */
