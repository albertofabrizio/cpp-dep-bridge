# v0.4.0-alpha Release Notes (draft)

## Behavior changes

1. Imported CMake targets normalize to package-level component identity.
2. Classification is evidence-gated and falls back to `unknown` for ambiguous cases.
3. Default ingestion no longer performs package-manager guessing from library paths.
4. CycloneDX output now includes deterministic `dependencies`.
5. CycloneDX metadata tool version is wired to `depbridge::version` and includes metadata subject when resolvable.

## Migration notes

- Expect component-id shifts for imported-target-heavy graphs.
- Expect fewer automatic `third_party`/`system` classifications without explicit evidence.
- SBOM consumers should handle populated `dependencies` and metadata subject.
