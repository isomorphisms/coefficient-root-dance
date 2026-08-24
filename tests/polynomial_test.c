#include "../app/src/main/cpp/polynomial.h"

#include <assert.h>
#include <complex.h>
#include <math.h>
#include <stdio.h>

static int close_complex(float complex a, float complex b, float tolerance) {
    return cabsf(a - b) < tolerance;
}

static void check_round_trip(
    int degree,
    const float complex expected[MAX_POLYNOMIAL_DEGREE],
    float tolerance
) {
    float complex roots[MAX_POLYNOMIAL_DEGREE] = {0};
    float complex coefficients[MAX_POLYNOMIAL_DEGREE] = {0};

    for (int index = 0; index < degree; ++index) {
        roots[index] = expected[index];
    }

    roots_to_coefficients(degree, roots, coefficients);
    assert(coefficients_to_roots(degree, coefficients, roots));
    for (int index = 0; index < degree; ++index) {
        assert(close_complex(roots[index], expected[index], tolerance));
    }
}

int main(void) {
    float complex quadratic[MAX_POLYNOMIAL_DEGREE] = {
        -1.0f + 0.6f * I,
         1.0f - 0.4f * I
    };
    float complex coefficients[MAX_POLYNOMIAL_DEGREE] = {0};

    roots_to_coefficients(2, quadratic, coefficients);
    assert(close_complex(coefficients[1], -0.2f * I, 1.0e-4f));
    assert(close_complex(coefficients[0], -0.76f + 1.0f * I, 1.0e-4f));
    check_round_trip(2, quadratic, 1.0e-4f);

    float complex cubic[MAX_POLYNOMIAL_DEGREE] = {
        -1.0f + 0.5f * I,
         0.25f - 0.75f * I,
         1.25f + 0.2f * I
    };
    check_round_trip(3, cubic, 2.0e-4f);

    float complex sextic[MAX_POLYNOMIAL_DEGREE] = {
        -1.4f + 0.2f * I,
        -0.8f - 0.9f * I,
        -0.1f + 1.1f * I,
         0.55f - 1.2f * I,
         1.15f + 0.75f * I,
         1.6f - 0.15f * I
    };
    check_round_trip(6, sextic, 1.0e-3f);

    float complex degree_twelve[MAX_POLYNOMIAL_DEGREE] = {
        -1.45f + 0.10f * I,
        -1.15f - 0.55f * I,
        -0.90f + 0.85f * I,
        -0.55f - 1.05f * I,
        -0.20f + 1.30f * I,
         0.15f - 1.35f * I,
         0.45f + 1.05f * I,
         0.75f - 0.85f * I,
         1.00f + 0.62f * I,
         1.20f - 0.42f * I,
         1.38f + 0.18f * I,
         0.05f + 0.22f * I
    };
    check_round_trip(12, degree_twelve, 4.0e-3f);

    float complex repeated[MAX_POLYNOMIAL_DEGREE] = {0};
    roots_to_coefficients(4, repeated, coefficients);
    float complex spread[MAX_POLYNOMIAL_DEGREE] = {
        0.08f + 0.00f * I,
        0.00f + 0.08f * I,
       -0.08f + 0.00f * I,
        0.00f - 0.08f * I
    };
    assert(coefficients_to_roots(4, coefficients, spread));
    for (int index = 0; index < 4; ++index) {
        assert(cabsf(spread[index]) < 3.0e-3f);
    }

    float complex moving[MAX_POLYNOMIAL_DEGREE] = {
        -1.2f + 0.1f * I,
         0.2f - 0.9f * I,
         1.1f + 0.4f * I
    };
    roots_to_coefficients(3, moving, coefficients);
    float complex previous_zero = moving[0];
    float complex previous_one = moving[1];
    float complex previous_two = moving[2];
    coefficients[0] += 0.015f - 0.01f * I;
    assert(coefficients_to_roots(3, coefficients, moving));
    assert(cabsf(moving[0] - previous_zero) < cabsf(moving[0] - previous_one));
    assert(cabsf(moving[0] - previous_zero) < cabsf(moving[0] - previous_two));
    assert(cabsf(moving[1] - previous_one) < cabsf(moving[1] - previous_zero));
    assert(cabsf(moving[2] - previous_two) < cabsf(moving[2] - previous_zero));

    puts("variable-degree polynomial conversion checks passed");
    return 0;
}
