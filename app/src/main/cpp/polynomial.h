#ifndef COEFFICIENT_ROOT_DANCE_POLYNOMIAL_H
#define COEFFICIENT_ROOT_DANCE_POLYNOMIAL_H

#include <complex.h>

enum {
    QUADRATIC_COEFFICIENT_COUNT = 2,
    QUADRATIC_ROOT_COUNT = 2
};

/*
 * Monic quadratic:
 *
 *     z^2 + coefficients[1] z + coefficients[0]
 *
 * The leading coefficient is fixed at 1, so two complex coefficient handles
 * and two complex roots contain exactly the same degrees of freedom.
 */
void roots_to_coefficients(
    const float complex roots[QUADRATIC_ROOT_COUNT],
    float complex coefficients[QUADRATIC_COEFFICIENT_COUNT]
);

/*
 * Recompute both roots after a coefficient move. roots[] is also the previous
 * root state; the assignment is chosen to minimize total root motion so the
 * two unnumbered root dots do not gratuitously swap identities while dragging.
 */
void coefficients_to_roots(
    const float complex coefficients[QUADRATIC_COEFFICIENT_COUNT],
    float complex roots[QUADRATIC_ROOT_COUNT]
);

#endif
