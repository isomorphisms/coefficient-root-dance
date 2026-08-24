#version 300 es
precision highp float;

const int MAX_POLYNOMIAL_DEGREE = 12;

uniform vec2 u_resolution;
uniform float u_half_height;
uniform vec2 u_coefficients[MAX_POLYNOMIAL_DEGREE];
uniform vec2 u_roots[MAX_POLYNOMIAL_DEGREE];
uniform int u_degree;
uniform int u_active_kind;
uniform int u_active_index;

out vec4 out_color;

float line_mask(float distance_in_pixels, float half_width) {
    return 1.0 - smoothstep(half_width, half_width + 1.25, distance_in_pixels);
}

float circle_mask(vec2 point, vec2 center, float radius) {
    return 1.0 - smoothstep(radius - 1.0, radius + 1.0, distance(point, center));
}

float ring_mask(vec2 point, vec2 center, float radius, float thickness) {
    float ring_distance = abs(distance(point, center) - radius);
    return 1.0 - smoothstep(thickness - 0.75, thickness + 0.75, ring_distance);
}

float segment_mask(vec2 point, vec2 first, vec2 second, float half_width) {
    vec2 direction = second - first;
    float denominator = dot(direction, direction);
    float amount = clamp(dot(point - first, direction) / denominator, 0.0, 1.0);
    vec2 nearest = first + amount * direction;
    return line_mask(distance(point, nearest), half_width);
}

float digit_mask(int digit, vec2 point) {
    float top = segment_mask(point, vec2(-5.0, 8.0), vec2(5.0, 8.0), 1.15);
    float upper_right = segment_mask(point, vec2(5.5, 7.0), vec2(5.5, 0.8), 1.15);
    float lower_right = segment_mask(point, vec2(5.5, -0.8), vec2(5.5, -7.0), 1.15);
    float bottom = segment_mask(point, vec2(-5.0, -8.0), vec2(5.0, -8.0), 1.15);
    float lower_left = segment_mask(point, vec2(-5.5, -0.8), vec2(-5.5, -7.0), 1.15);
    float upper_left = segment_mask(point, vec2(-5.5, 7.0), vec2(-5.5, 0.8), 1.15);
    float middle = segment_mask(point, vec2(-5.0, 0.0), vec2(5.0, 0.0), 1.15);

    float result = 0.0;
    if (digit != 1 && digit != 4) {
        result = max(result, top);
    }
    if (digit != 5 && digit != 6) {
        result = max(result, upper_right);
    }
    if (digit != 2) {
        result = max(result, lower_right);
    }
    if (digit != 1 && digit != 4 && digit != 7) {
        result = max(result, bottom);
    }
    if (digit == 0 || digit == 2 || digit == 6 || digit == 8) {
        result = max(result, lower_left);
    }
    if (digit == 0 || digit == 4 || digit == 5 || digit == 6 || digit == 8 || digit == 9) {
        result = max(result, upper_left);
    }
    if (digit != 0 && digit != 1 && digit != 7) {
        result = max(result, middle);
    }
    return result;
}

float number_mask(int number, vec2 point) {
    if (number < 10) {
        return digit_mask(number, point);
    }

    int tens = number / 10;
    int ones = number - 10 * tens;
    float left = digit_mask(tens, point + vec2(6.5, 0.0));
    float right = digit_mask(ones, point - vec2(6.5, 0.0));
    return max(left, right);
}

vec2 complex_to_screen(vec2 value, bool right_side) {
    float side_width = 0.5 * u_resolution.x;
    float aspect = side_width / u_resolution.y;
    float side_start = right_side ? side_width : 0.0;

    float normalized_x = value.x / (u_half_height * aspect);
    float normalized_y = value.y / u_half_height;

    return vec2(
        side_start + 0.5 * (normalized_x + 1.0) * side_width,
        0.5 * (normalized_y + 1.0) * u_resolution.y
    );
}

vec2 screen_to_complex(vec2 point) {
    float side_width = 0.5 * u_resolution.x;
    bool right_side = point.x >= side_width;
    float local_x = point.x - (right_side ? side_width : 0.0);
    float aspect = side_width / u_resolution.y;

    return vec2(
        (2.0 * local_x / side_width - 1.0) * u_half_height * aspect,
        (2.0 * point.y / u_resolution.y - 1.0) * u_half_height
    );
}

bool roots_share_marker(int first, int second) {
    vec2 first_center = complex_to_screen(u_roots[first], true);
    vec2 second_center = complex_to_screen(u_roots[second], true);
    return distance(first_center, second_center) <= 2.0;
}

void main() {
    vec2 point = gl_FragCoord.xy;
    bool right_side = point.x >= 0.5 * u_resolution.x;
    vec2 value = screen_to_complex(point);

    vec3 left_background = vec3(0.965, 0.950, 0.925);
    vec3 right_background = vec3(0.930, 0.950, 0.970);
    vec3 color = right_side ? right_background : left_background;

    float axis_x = line_mask(abs(value.x) * u_resolution.y / (2.0 * u_half_height), 0.7);
    float axis_y = line_mask(abs(value.y) * u_resolution.y / (2.0 * u_half_height), 0.7);
    color = mix(color, vec3(0.67), max(axis_x, axis_y));

    float divider = line_mask(abs(point.x - 0.5 * u_resolution.x), 1.2);
    color = mix(color, vec3(0.22), divider);

    vec3 coefficient_color = vec3(0.86, 0.34, 0.12);
    vec3 root_color = vec3(0.08, 0.32, 0.72);

    for (int index = 0; index < MAX_POLYNOMIAL_DEGREE; ++index) {
        if (index >= u_degree) {
            continue;
        }

        vec2 center = complex_to_screen(u_coefficients[index], false);
        float handle = circle_mask(point, center, 19.0);
        color = mix(color, coefficient_color, handle);

        float selection_ring = 0.0;
        if (u_active_kind == 1 && u_active_index == index) {
            selection_ring = ring_mask(point, center, 25.0, 2.5);
        }
        color = mix(color, vec3(0.15), selection_ring);

        float digit = number_mask(index, point - center) * handle;
        color = mix(color, vec3(1.0), digit);
    }

    for (int index = 0; index < MAX_POLYNOMIAL_DEGREE; ++index) {
        if (index >= u_degree) {
            continue;
        }

        bool representative = true;
        for (int previous = 0; previous < MAX_POLYNOMIAL_DEGREE; ++previous) {
            if (previous >= u_degree) {
                continue;
            }
            if (previous < index && roots_share_marker(index, previous)) {
                representative = false;
            }
        }
        if (!representative) {
            continue;
        }

        int multiplicity = 1;
        bool cluster_active = u_active_kind == 2 && u_active_index == index;
        for (int other = 0; other < MAX_POLYNOMIAL_DEGREE; ++other) {
            if (other >= u_degree) {
                continue;
            }
            if (other != index && roots_share_marker(index, other)) {
                multiplicity += 1;
                if (u_active_kind == 2 && u_active_index == other) {
                    cluster_active = true;
                }
            }
        }

        vec2 center = complex_to_screen(u_roots[index], true);
        float dot = circle_mask(point, center, 12.0);
        color = mix(color, root_color, dot);

        for (int ring_index = 1; ring_index < MAX_POLYNOMIAL_DEGREE; ++ring_index) {
            if (ring_index < multiplicity) {
                float radius = 12.0 + 7.0 * float(ring_index);
                float repeated_ring = ring_mask(point, center, radius, 2.25);
                color = mix(color, root_color, repeated_ring);
            }
        }

        if (cluster_active) {
            float active_radius = 20.0 + 7.0 * float(multiplicity - 1);
            float selection_ring = ring_mask(point, center, active_radius, 2.5);
            color = mix(color, vec3(0.15), selection_ring);
        }
    }

    float side_width = 0.5 * u_resolution.x;
    vec2 minus_center = vec2(44.0, u_resolution.y - 44.0);
    vec2 plus_center = vec2(side_width - 44.0, u_resolution.y - 44.0);
    vec2 degree_center = vec2(0.5 * side_width, u_resolution.y - 44.0);

    float minus_button = circle_mask(point, minus_center, 24.0);
    float plus_button = circle_mask(point, plus_center, 24.0);
    color = mix(color, vec3(0.82), max(minus_button, plus_button));

    float minus_sign = segment_mask(
        point - minus_center,
        vec2(-8.0, 0.0),
        vec2(8.0, 0.0),
        1.6
    ) * minus_button;
    float plus_horizontal = segment_mask(
        point - plus_center,
        vec2(-8.0, 0.0),
        vec2(8.0, 0.0),
        1.6
    );
    float plus_vertical = segment_mask(
        point - plus_center,
        vec2(0.0, -8.0),
        vec2(0.0, 8.0),
        1.6
    );
    float plus_sign = max(plus_horizontal, plus_vertical) * plus_button;
    color = mix(color, vec3(0.15), max(minus_sign, plus_sign));

    float degree_digit = number_mask(u_degree, point - degree_center);
    color = mix(color, vec3(0.15), degree_digit);

    out_color = vec4(color, 1.0);
}
