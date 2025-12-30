This is a minimal CMake project used for integration testing.

Targets:
- foo (static library)
- app (executable)

The integration test runs cpp-dep-bridge on this project and
verifies that:
- targets are discovered
- components are normalized
- a CycloneDX SBOM is produced deterministically
