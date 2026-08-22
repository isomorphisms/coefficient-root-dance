#include "polynomial.h"

#include <math.h>

static float distance_squared(float complex a, float complex b) {
    float complex difference = a - b;
    float real_part = crealf(difference);
    float imaginary_part = cimagf(difference);
    return real_part * real_part + imaginary_part * imaginary_part;
}

void roots_to_coefficients(
    const float complex roots[QUADRATIC_ROOT_COUNT],
    float complex coefficients[QUADRATIC_COEFFICIENT_COUNT]
) {
    coefficients[1] = -(roots[0] + roots[1]);
    coefficients[0] = roots[0] * roots[1];
}

void coefficients_to_roots(
    const float complex coefficients[QUADRATIC_COEFFICIENT_COUNT],
    float complex roots[QUADRATIC_ROOT_COUNT]
) {
    float complex previous_0 = roots[0];
    float complex previous_1 = roots[1];

    float complex c0 = coefficients[0];
    float complex c1 = coefficients[1];
    float complex discriminant = c1 * c1 - 4.0f * c0;
    float complex square_root = csqrtf(discriminant);

    float complex candidate_0 = (-c1 + square_root) * 0.5f;
    float complex candidate_1 = (-c1 - square_root) * 0.5f;

    float same_order =
        distance_squared(candidate_0, previous_0) +
        distance_squared(candidate_1, previous_1);
    float swapped_order =
        distance_squared(candidate_1, previous_0) +
        distance_squared(candidate_0, previous_1);

    if (swapped_order < same_order) {
        roots[0] = candidate_1;
        roots[1] = candidate_0;
    } else {
        roots[0] = candidate_0;
        roots[1] = candidate_1;
    }
}
