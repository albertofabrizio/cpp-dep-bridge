# Dependency Model

This document describes the **internal dependency model** used by **cpp-dep-bridge**.

The model is intentionally **tool-agnostic**, **build-graph–driven**, and designed to be stable over time.  
All ingestion layers (CMake, Conan, vcpkg, etc.) *adapt to this model* — the model never adapts to them.

---

## Design principles

### 1. The build graph is the source of truth
Dependency information is derived from what the build **actually links and uses**, not from manifests or intent.

Manifests and lockfiles are treated as **evidence**, never as authoritative truth.

---

### 2. Targets ≠ Components
The model strictly separates:
- **Build targets** (CMake concepts)
- **Components** (SBOM concepts)

This avoids leaking build-system semantics into SBOM output and allows multiple build targets to map to the same external component.

---

### 3. Deterministic identity
All entities have **stable, reproducible identifiers**:
- No random UUIDs
- No machine-specific paths in IDs
- Identical inputs → identical graphs → identical SBOMs

This is essential for diffing, auditing, and CI reproducibility.

---

### 4. Provenance is first-class
Every fact in the model records **where it came from**.

Users must always be able to answer:
> *“Why is this dependency here?”*

---

## Core entities

### ProjectGraph

`ProjectGraph` is the root container for a single resolution run.

It contains:
- all build targets
- all resolved components
- dependency edges between them
- resolution context and diagnostics

Conceptually:

```
ProjectGraph
 ├── BuildTargets
 ├── Components
 ├── DependencyEdges
 └── ResolutionContext
```

---

### ResolutionContext

Captures the environment in which resolution occurred.

Includes:
- project root directory
- build directory
- generator (Ninja, Visual Studio, etc.)
- selected environment variables
- ingestion sources

This data is used for:
- traceability
- debugging
- avoiding false diffs across environments

---

## BuildTarget

Represents a **build-system target** (e.g. CMake target).

A `BuildTarget` describes:
- what is being built
- how it is built
- what it links against

Key properties:
- stable `TargetId`
- target kind (executable, static library, shared library, interface)
- compile options, definitions, include directories
- produced artifacts (if known)
- provenance sources

A build target may depend on:
- other build targets
- external components

---

## Component

Represents a **logical dependency** suitable for SBOM emission.

Examples:
- third-party libraries
- system libraries
- frameworks
- header-only packages
- external tools

A `Component` is defined by:
- name and optional namespace
- optional version
- type (library, framework, tool, system, etc.)
- optional SBOM identifiers (purl, CPE)
- license information
- checksums (when available)
- provenance sources

### Important rule
> A component may exist **without a known version**.

Missing information is preserved explicitly — it is never guessed.

---

## DependencyEdge

A directed edge representing a dependency relationship.

```
BuildTarget ──▶ Component
BuildTarget ──▶ BuildTarget
```

Each edge records:
- dependency scope (runtime, build, test, optional)
- linkage type (static, shared, interface)
- provenance
- raw build-system evidence (for debugging)

Edges are **directional** and **explicit**.

---

## Artifact

Represents a concrete file involved in the build:
- static libraries
- shared libraries
- executables
- optional debug symbols

Artifacts may have:
- normalized paths
- checksums
- provenance

Artifacts are optional but improve SBOM fidelity when available.

---

## Identity model

### TargetId
Derived from:
- build system
- project namespace
- target name
- optional configuration

Target IDs are stable across machines.

---

### ComponentId
Derived from canonical attributes:
- component type
- namespace
- name
- version (or empty)

If available, **purl** is used as the primary identity seed.

No random identifiers are ever generated.

---

## Normalization pipeline

All ingestion layers feed into a common normalization process:

1. **Token normalization**
   - normalize paths
   - normalize library names
   - resolve aliases where possible

2. **Target-first resolution**
   - prefer build targets over raw libraries
   - convert to components only when appropriate

3. **Component canonicalization**
   - unify duplicates
   - preserve missing data explicitly

4. **Provenance attachment**
   - every entity and edge records its evidence

---

## Tool isolation

The model itself:
- does **not** depend on CMake
- does **not** depend on Conan or vcpkg
- does **not** depend on SBOM formats

Adapters are responsible for translating tool-specific data into the model.

---

## Why this model exists

This design enables:
- accurate SBOM generation
- dependency submission to CI ecosystems
- reproducible diffs across environments
- future ingestion sources without refactoring

Most importantly, it ensures that **cpp-dep-bridge reports reality, not intent**.

---

## Stability guarantee

The dependency model is considered **stable API surface**.

Breaking changes:
- will be rare
- will be versioned
- will be documented explicitly

Everything else in the project may evolve — this model is the foundation.
