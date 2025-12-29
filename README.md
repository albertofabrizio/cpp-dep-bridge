# cpp-dep-bridge

**cpp-dep-bridge** is a build-aware C++ dependency extraction tool.

It analyzes **real CMake build graphs** (not just manifests) and produces:

- accurate dependency graphs
- machine-readable SBOMs (CycloneDX first)
- optional dependency submission data for CI systems

The goal is **deterministic, auditable dependency metadata** for modern C++ projects.

---

## Non-goals

This project is **not**:

- a build system
- a package manager
- a vulnerability scanner
- a CMake replacement or wrapper

It consumes existing builds and reports what they _actually_ use.

---

## Requirements

- **CMake ≥ 3.25**
- **C++20 compiler**
- Ninja (recommended)
- Dependencies provided via **vcpkg or Conan**:
  - `fmt`
  - `nlohmann_json`
  - (optional) `CLI11`

---

## Getting started (preset-first)

This project is **preset-only**.  
Do not invoke raw `cmake ..` commands.

### 1. Clone

```bash
git clone https://github.com/albertofabrizio/cpp-dep-bridge.git
cd cpp-dep-bridge
```

### 2. Configure (development build)

```bash
cmake --preset dev
```

### 3. Build

```bash
cmake --build --preset dev
```

### 4. Run tests (optional)

```bash
ctest --preset ci
```

---

## Available presets

| Preset    | Purpose                                               |
| --------- | ----------------------------------------------------- |
| `dev`     | Default development build (RelWithDebInfo)            |
| `debug`   | Debug build                                           |
| `asan`    | Debug + Address/UB sanitizers                         |
| `release` | Optimized release build                               |
| `ci`      | Strict build (warnings as errors, sanitizers enabled) |

To list presets:

```bash
cmake --list-presets
```

---

## Preset philosophy

Presets are **part of the project contract**:

- CI uses the same presets as developers
- IDEs auto-detect them
- Reproducing issues locally should never require guessing flags

Local, machine-specific overrides belong in `CMakeUserPresets.json`  
(which is intentionally **not committed**).

---

## Project structure (high level)

```text
include/   → public headers
src/       → implementation
tests/     → unit and integration tests
cmake/     → CMake helper modules
docs/      → architecture and design notes
```

---

## License

Apache License 2.0  
See `LICENSE` and `NOTICE` for details.

---

## Status

**Early development (v0.1 planned).**

Current focus:

- CMake File API ingestion
- deterministic dependency graph construction
- CycloneDX SBOM emission

APIs and output formats may change until the first tagged release.
