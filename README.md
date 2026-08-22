# coefficient-root-dance

A native Android touch prototype for moving between polynomial coefficients and roots.

The first slice is deliberately small: a **monic quadratic**

`z² + c₁z + c₀`

so the two movable coefficient values and the two roots carry the same amount of information.

## Interaction

- Left half: numbered coefficient handles `0` and `1`.
- Right half: the two root dots.
- Drag either coefficient handle and both roots update immediately.
- Drag either root dot and both coefficients update immediately.
- The leading coefficient is fixed at `1` for this prototype.
- Repeated roots are shown as concentric circles: a double root is a dot with one surrounding circle.

The Android shell uses the same basic `NativeActivity` + `AInputEvent` + EGL/GLES touch/render loop already proven in Wegert. The polynomial conversion is isolated in `polynomial.c`.

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
