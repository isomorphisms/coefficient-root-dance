#ifndef COEFFICIENT_ROOT_DANCE_POLYNOMIAL_H
#define COEFFICIENT_ROOT_DANCE_POLYNOMIAL_H

#include <complex.h>
#include <stdbool.h>

enum {
    MIN_POLYNOMIAL_DEGREE = 1,
    MAX_POLYNOMIAL_DEGREE = 12
};

/*
 * Monic polynomial of runtime degree n:
 *
 *     z^n + coefficients[n - 1] z^(n - 1) + ... + coefficients[0]
 *
 * The leading coefficient is fixed at 1.  The active prefixes of roots[] and
 * coefficients[] each therefore contain exactly n complex degrees of freedom.
 */
void roots_to_coefficients(
    int degree,
    const float complex roots[MAX_POLYNOMIAL_DEGREE],
    float complex coefficients[MAX_POLYNOMIAL_DEGREE]
);

/*
 * Recompute the active roots after a coefficient move. roots[] is also the
 * previous root state.  After solving, a minimum-cost matching keeps each
 * unnumbered dot as close as possible to its previous position, avoiding
 * gratuitous root swaps while dragging.
 *
 * Returns false for an out-of-range degree or a non-finite numerical solve.
 */
bool coefficients_to_roots(
    int degree,
    const float complex coefficients[MAX_POLYNOMIAL_DEGREE],
    float complex roots[MAX_POLYNOMIAL_DEGREE]
);

#endif
