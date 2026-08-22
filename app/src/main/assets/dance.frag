#version 300 es
precision highp float;

uniform vec2 u_resolution;
uniform float u_half_height;
uniform vec2 u_coefficients[2];
uniform vec2 u_roots[2];
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

float zero_digit(vec2 point) {
    vec2 q = point / vec2(7.0, 10.0);
    float ellipse = length(q);
    return 1.0 - smoothstep(0.10, 0.16, abs(ellipse - 0.62));
}

float one_digit(vec2 point) {
    float stem = line_mask(abs(point.x), 1.25) *
                 step(-8.5, point.y) * step(point.y, 8.5);
    float foot = line_mask(abs(point.y + 8.0), 1.1) *
                 step(-5.0, point.x) * step(point.x, 5.0);
    float cap_distance = abs(point.x + point.y * 0.45 + 3.0) / 1.1;
    float cap = line_mask(cap_distance, 1.0) *
                step(2.0, point.y) * step(point.y, 8.5);
    return max(stem, max(foot, cap));
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

    for (int index = 0; index < 2; ++index) {
        vec2 center = complex_to_screen(u_coefficients[index], false);
        float handle = circle_mask(point, center, 19.0);
        color = mix(color, coefficient_color, handle);

        float active = 0.0;
        if (u_active_kind == 1 && u_active_index == index) {
            active = ring_mask(point, center, 25.0, 2.5);
        }
        color = mix(color, vec3(0.15), active);

        vec2 digit_point = point - center;
        float digit = index == 0 ? zero_digit(digit_point) : one_digit(digit_point);
        digit *= handle;
        color = mix(color, vec3(1.0), digit);
    }

    for (int index = 0; index < 2; ++index) {
        vec2 center = complex_to_screen(u_roots[index], true);
        float dot = circle_mask(point, center, 12.0);
        color = mix(color, root_color, dot);

        float active = 0.0;
        if (u_active_kind == 2 && u_active_index == index) {
            active = ring_mask(point, center, 20.0, 2.5);
        }
        color = mix(color, vec3(0.15), active);
    }

    out_color = vec4(color, 1.0);
}
