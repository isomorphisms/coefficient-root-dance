#include "../app/src/main/cpp/polynomial.h"

#include <assert.h>
#include <complex.h>
#include <math.h>
#include <stdio.h>

static int close_complex(float complex a, float complex b) {
    return cabsf(a - b) < 1.0e-4f;
}

int main(void) {
    float complex roots[QUADRATIC_ROOT_COUNT] = {
        -1.0f + 0.6f * I,
         1.0f - 0.4f * I
    };
    float complex coefficients[QUADRATIC_COEFFICIENT_COUNT];

    roots_to_coefficients(roots, coefficients);
    assert(close_complex(coefficients[1], -0.2f * I));
    assert(close_complex(coefficients[0], -0.76f + 1.0f * I));

    float complex previous_0 = roots[0];
    float complex previous_1 = roots[1];
    coefficients_to_roots(coefficients, roots);
    assert(close_complex(roots[0], previous_0));
    assert(close_complex(roots[1], previous_1));

    coefficients[1] = -2.0f + 0.0f * I;
    coefficients[0] =  1.0f + 0.0f * I;
    coefficients_to_roots(coefficients, roots);
    assert(close_complex(roots[0], 1.0f + 0.0f * I));
    assert(close_complex(roots[1], 1.0f + 0.0f * I));

    puts("polynomial conversion checks passed");
    return 0;
}
