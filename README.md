# coefficient-root-dance

A native Android touch prototype for moving between polynomial coefficients and roots.

The interaction now uses a runtime-degree **monic polynomial**

`zⁿ + cₙ₋₁zⁿ⁻¹ + ... + c₁z + c₀`

with the leading coefficient fixed at `1`. The current mobile slice supports degrees 1 through 12; that bound is a renderer/interaction limit, not part of the polynomial model.

## Interaction

- Left half: numbered coefficient handles `0` through `n - 1`.
- Right half: `n` unnumbered root dots.
- Drag any coefficient handle and all roots update immediately.
- Drag any root dot and all coefficients update immediately.
- `−` and `+` at the top of the coefficient side change the degree; the number between them is the current degree.
- Increasing the degree multiplies the current polynomial by `z`, adding a root at the origin without disturbing the existing roots.
- Decreasing the degree removes the root nearest the origin, a geometric rule that does not impose visible root numbering.
- Repeated roots are shown as concentric circles, one circle per multiplicity level beyond the first dot.

Coefficient-to-root updates use a generic simultaneous root solve and then a minimum-motion assignment against the previous frame so the unnumbered root dots avoid gratuitous swaps while dragging. Root-to-coefficient updates multiply the linear factors directly.

The Android shell uses the same basic `NativeActivity` + `AInputEvent` + EGL/GLES touch/render loop already proven in Wegert. The polynomial conversion remains isolated in `polynomial.c`.

## Build

Requires Android SDK 36, NDK `29.0.14206865`, CMake 3.22.1, and Gradle 9.5+.

```sh
gradle :app:assembleDebug
```

The debug APK is written to `app/build/outputs/apk/debug/app-debug.apk`.

## Math check

```sh
cc -std=c11 -Wall -Wextra -Werror \
  tests/polynomial_test.c app/src/main/cpp/polynomial.c -lm \
  -o /tmp/polynomial_test
/tmp/polynomial_test
```
