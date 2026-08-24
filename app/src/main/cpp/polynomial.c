#include "polynomial.h"

#include <float.h>
#include <math.h>
#include <stddef.h>

#define ROOT_ITERATIONS 192
#define ROOT_TOLERANCE 1.0e-5f
#define ROOT_SEPARATION 1.0e-3f

static float distance_squared(float complex a, float complex b) {
    float complex difference = a - b;
    float real_part = crealf(difference);
    float imaginary_part = cimagf(difference);
    return real_part * real_part + imaginary_part * imaginary_part;
}

static bool finite_complex(float complex value) {
    return isfinite(crealf(value)) && isfinite(cimagf(value));
}

static float complex evaluate_polynomial(
    int degree,
    const float complex coefficients[MAX_POLYNOMIAL_DEGREE],
    float complex value
) {
    float complex result = 1.0f + 0.0f * I;
    for (int exponent = degree - 1; exponent >= 0; --exponent) {
        result = result * value + coefficients[exponent];
    }
    return result;
}

static float root_bound(
    int degree,
    const float complex coefficients[MAX_POLYNOMIAL_DEGREE]
) {
    float largest = 0.0f;
    for (int index = 0; index < degree; ++index) {
        float magnitude = cabsf(coefficients[index]);
        if (magnitude > largest) {
            largest = magnitude;
        }
    }
    return 1.0f + largest;
}

static void prepare_guesses(
    int degree,
    float radius,
    const float complex previous[MAX_POLYNOMIAL_DEGREE],
    float complex guesses[MAX_POLYNOMIAL_DEGREE]
) {
    const float tau = 6.2831853071795864769f;
    float nudge = ROOT_SEPARATION * fmaxf(1.0f, radius);

    for (int index = 0; index < degree; ++index) {
        if (finite_complex(previous[index])) {
            guesses[index] = previous[index];
        } else {
            float angle = tau * ((float)index + 0.25f) / (float)degree;
            guesses[index] = radius * (cosf(angle) + sinf(angle) * I);
        }
    }

    for (int first = 0; first < degree; ++first) {
        for (int second = 0; second < first; ++second) {
            if (cabsf(guesses[first] - guesses[second]) < nudge) {
                float angle = tau * ((float)first + 0.5f) / (float)degree;
                guesses[first] += nudge * (cosf(angle) + sinf(angle) * I);
            }
        }
    }
}

static void prepare_circle_guesses(
    int degree,
    float radius,
    float complex guesses[MAX_POLYNOMIAL_DEGREE]
) {
    const float tau = 6.2831853071795864769f;
    for (int index = 0; index < degree; ++index) {
        float angle = tau * ((float)index + 0.25f) / (float)degree;
        guesses[index] = radius * (cosf(angle) + sinf(angle) * I);
    }
}

static bool iterate_roots(
    int degree,
    float radius,
    const float complex coefficients[MAX_POLYNOMIAL_DEGREE],
    float complex current[MAX_POLYNOMIAL_DEGREE],
    float complex solved[MAX_POLYNOMIAL_DEGREE]
) {
    float complex next[MAX_POLYNOMIAL_DEGREE];

    for (int iteration = 0; iteration < ROOT_ITERATIONS; ++iteration) {
        float largest_correction = 0.0f;

        for (int index = 0; index < degree; ++index) {
            float complex denominator = 1.0f + 0.0f * I;
            for (int other = 0; other < degree; ++other) {
                if (other != index) {
                    denominator *= current[index] - current[other];
                }
            }

            if (cabsf(denominator) < FLT_EPSILON) {
                float angle =
                    6.2831853071795864769f *
                    ((float)index + 0.5f) /
                    (float)degree;
                current[index] +=
                    ROOT_SEPARATION *
                    fmaxf(1.0f, radius) *
                    (cosf(angle) + sinf(angle) * I);

                denominator = 1.0f + 0.0f * I;
                for (int other = 0; other < degree; ++other) {
                    if (other != index) {
                        denominator *= current[index] - current[other];
                    }
                }
            }

            float complex correction =
                evaluate_polynomial(degree, coefficients, current[index]) /
                denominator;
            next[index] = current[index] - correction;

            if (!finite_complex(next[index])) {
                return false;
            }

            float correction_size = cabsf(correction);
            if (correction_size > largest_correction) {
                largest_correction = correction_size;
            }
        }

        for (int index = 0; index < degree; ++index) {
            current[index] = next[index];
        }

        if (largest_correction < ROOT_TOLERANCE) {
            break;
        }
    }

    for (int index = 0; index < degree; ++index) {
        solved[index] = current[index];
    }
    return true;
}

static bool solve_roots(
    int degree,
    const float complex coefficients[MAX_POLYNOMIAL_DEGREE],
    const float complex previous[MAX_POLYNOMIAL_DEGREE],
    float complex solved[MAX_POLYNOMIAL_DEGREE]
) {
    if (degree == 1) {
        solved[0] = -coefficients[0];
        return finite_complex(solved[0]);
    }

    float radius = root_bound(degree, coefficients);
    float complex guesses[MAX_POLYNOMIAL_DEGREE];

    prepare_guesses(degree, radius, previous, guesses);
    if (iterate_roots(degree, radius, coefficients, guesses, solved)) {
        return true;
    }

    prepare_circle_guesses(degree, radius, guesses);
    return iterate_roots(degree, radius, coefficients, guesses, solved);
}

static int bit_count(unsigned int mask) {
    int count = 0;
    while (mask != 0u) {
        count += (int)(mask & 1u);
        mask >>= 1u;
    }
    return count;
}

static void match_to_previous(
    int degree,
    const float complex previous[MAX_POLYNOMIAL_DEGREE],
    const float complex candidates[MAX_POLYNOMIAL_DEGREE],
    float complex roots[MAX_POLYNOMIAL_DEGREE]
) {
    for (int index = 0; index < degree; ++index) {
        if (!finite_complex(previous[index])) {
            for (int copy_index = 0; copy_index < degree; ++copy_index) {
                roots[copy_index] = candidates[copy_index];
            }
            return;
        }
    }

    enum { MATCH_STATE_COUNT = 1 << MAX_POLYNOMIAL_DEGREE };
    float cost[MATCH_STATE_COUNT];
    int chosen[MATCH_STATE_COUNT];
    unsigned int parent[MATCH_STATE_COUNT];
    unsigned int full_mask = (1u << (unsigned int)degree) - 1u;

    for (unsigned int mask = 0; mask <= full_mask; ++mask) {
        cost[mask] = INFINITY;
        chosen[mask] = -1;
        parent[mask] = 0u;
    }
    cost[0] = 0.0f;

    for (unsigned int mask = 0; mask <= full_mask; ++mask) {
        if (!isfinite(cost[mask])) {
            continue;
        }

        int previous_index = bit_count(mask);
        if (previous_index >= degree) {
            continue;
        }

        for (int candidate_index = 0; candidate_index < degree; ++candidate_index) {
            unsigned int bit = 1u << (unsigned int)candidate_index;
            if ((mask & bit) != 0u) {
                continue;
            }

            unsigned int next_mask = mask | bit;
            float next_cost =
                cost[mask] +
                distance_squared(
                    previous[previous_index],
                    candidates[candidate_index]
                );
            if (next_cost < cost[next_mask]) {
                cost[next_mask] = next_cost;
                chosen[next_mask] = candidate_index;
                parent[next_mask] = mask;
            }
        }
    }

    unsigned int mask = full_mask;
    for (int previous_index = degree - 1; previous_index >= 0; --previous_index) {
        int candidate_index = chosen[mask];
        roots[previous_index] = candidates[candidate_index];
        mask = parent[mask];
    }
}

void roots_to_coefficients(
    int degree,
    const float complex roots[MAX_POLYNOMIAL_DEGREE],
    float complex coefficients[MAX_POLYNOMIAL_DEGREE]
) {
    float complex expanded[MAX_POLYNOMIAL_DEGREE + 1] = {0};
    expanded[0] = 1.0f + 0.0f * I;

    int current_degree = 0;
    for (int root_index = 0; root_index < degree; ++root_index) {
        float complex root = roots[root_index];
        for (int exponent = current_degree + 1; exponent >= 1; --exponent) {
            expanded[exponent] =
                expanded[exponent - 1] - root * expanded[exponent];
        }
        expanded[0] *= -root;
        current_degree += 1;
    }

    for (int exponent = 0; exponent < degree; ++exponent) {
        coefficients[exponent] = expanded[exponent];
    }
    for (int exponent = degree; exponent < MAX_POLYNOMIAL_DEGREE; ++exponent) {
        coefficients[exponent] = 0.0f + 0.0f * I;
    }
}

bool coefficients_to_roots(
    int degree,
    const float complex coefficients[MAX_POLYNOMIAL_DEGREE],
    float complex roots[MAX_POLYNOMIAL_DEGREE]
) {
    if (degree < MIN_POLYNOMIAL_DEGREE || degree > MAX_POLYNOMIAL_DEGREE) {
        return false;
    }

    float complex previous[MAX_POLYNOMIAL_DEGREE];
    float complex candidates[MAX_POLYNOMIAL_DEGREE];
    for (int index = 0; index < degree; ++index) {
        previous[index] = roots[index];
    }

    if (!solve_roots(degree, coefficients, previous, candidates)) {
        return false;
    }

    match_to_previous(degree, previous, candidates, roots);
    return true;
}
