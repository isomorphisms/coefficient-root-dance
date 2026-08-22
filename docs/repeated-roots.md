# Repeated roots

Root markers encode multiplicity with concentric circles.

- A simple root is one dot.
- A double root is the dot plus one surrounding circle.
- The shader is written as a multiplicity loop so increasing `ROOT_COUNT` later extends naturally to further nested circles.
- Roots within two screen pixels are treated as the same displayed root. This tolerance absorbs floating-point noise without merging visibly distinct roots.

The current polynomial model is quadratic, so multiplicity is currently at most two.
