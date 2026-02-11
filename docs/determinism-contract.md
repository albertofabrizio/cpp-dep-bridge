# Determinism Contract (v0.4.0-alpha)

## Ordering rules

- File API index selection: choose lexicographically greatest `index-*.json` filename.
- Components: sorted by `ComponentId` for SBOM emission.
- CycloneDX dependencies:
  - dependency objects sorted by `ref` (component id),
  - each `dependsOn` list sorted lexicographically.

## Identity and collision policy

- Component IDs are derived from canonical identity keys.
- Rebuild/merge code verifies canonical-key consistency when IDs collide.
- Silent merge is forbidden when canonical keys differ.

## Prohibited nondeterminism sources

- Filesystem mtime-based selection.
- Iteration-order dependence from unordered containers without sorting at output boundaries.
