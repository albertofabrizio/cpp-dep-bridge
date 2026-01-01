# Classification & Normalization Model

This document defines the **conceptual model, rules, and precedence** used by **cpp-dep-bridge** to turn raw build data into a deterministic dependency graph and SBOM.

It is the *source of truth* for how components are normalized, classified, filtered, and emitted.

---

## 1. Core Concepts

### 1.1 Build Targets

A **BuildTarget** represents a CMake target as seen by the CMake File API.

Examples:
- `depbridge_core`
- `fmt::fmt`
- `ALL_BUILD` (generator-provided)

Targets are **not** SBOM components by default.
They are *sources of evidence* that produce dependencies.

---

### 1.2 Components

A **Component** represents a real dependency that may appear in an SBOM.

Each component has:
- a **stable ComponentId**
- a **normalized name**
- a **type** (library, executable, tool, system, …)
- an **origin** (project-local, third-party, system, unknown)
- supporting **evidence** (`SourceRef`s)

Components are derived from:
- linker tokens
- imported targets
- library paths

---

## 2. Normalization Model

Normalization happens **before classification**.

### 2.1 Token Normalization

Raw linker tokens are normalized as follows:

| Input | Normalized Name |
|------|------------------|
| `-lfmt` | `fmt` |
| `D:/vcpkg/.../fmtd.lib` | `fmt` |
| `fmt::fmt` | `fmt` |
| `libssl.so` | `ssl` |

Rules:
- path separators normalized
- extensions stripped (`.lib`, `.a`, `.so`, `.dylib`)
- `lib` prefix stripped on Unix
- debug suffixes (`d`) stripped when detected

---

### 2.2 Imported Targets

Imported CMake targets (`foo::bar`) are **collapsed** into a single component:

- canonical name: namespace (`foo`)
- original target preserved as evidence

This ensures:
- `fmt::fmt` → `fmt`
- no duplication between target-based and path-based detection

---

### 2.3 Component Identity

Component identity is derived from **semantic fields only**:

```
(type, namespace, name, version, purl)
```

Build variants (Debug/Release) **do not affect identity**.

---

## 3. Classification Model

Classification assigns a **ComponentOrigin**.

### 3.1 Origins

| Origin | Meaning |
|------|--------|
| `project_local` | Built by the current project |
| `third_party` | External dependency |
| `system` | OS / toolchain provided |
| `unknown` | Insufficient evidence |

---

### 3.2 Classification Precedence (STRICT)

Classification is applied in this exact order:

1. **Project-local**
2. **System**
3. **Third-party**
4. **Unknown** (default)

Once a component is classified, **it is never downgraded**.

---

### 3.3 Project-local Classification

A component is `project_local` if:

- its name matches a build target
- OR it originates from a project build artifact

Evidence sources:
- `BuildTarget`
- local artifact paths

---

### 3.4 System Classification

A component is `system` if:

- its name matches known system libraries
  - `kernel32`, `user32`, `pthread`, `dl`, …
- OR it originates from system include/lib directories

System classification **does not override project-local**.

---

### 3.5 Third-party Classification

A component is `third_party` if:

- it originates from a package manager path
  - vcpkg
  - Conan
- OR it is an imported CMake target
- OR it has an explicit third-party `SourceRef`

Third-party classification **does not override project-local or system**.

---

## 4. Filtering Model

Filtering is applied **after classification**.

Filter options:

| Option | Effect |
|------|-------|
| `include_system` | Keep system components |
| `include_project_local` | Keep project-local components |

Filtering:
- removes components
- prunes edges pointing to removed components

---

## 5. Build-Variant Normalization

Build variants (Debug / Release):

- do **not** create separate components
- are detected and collapsed
- may be recorded as evidence only

Examples:
- `fmtd.lib` + `fmt.lib` → single `fmt` component

---

## 6. SBOM Emission

The CycloneDX writer:

- emits **only normalized components**
- emits one component per ComponentId
- preserves origin as a property

Example:

```json
{
  "name": "fmt",
  "type": "library",
  "properties": [
    { "name": "depbridge:origin", "value": "third-party" }
  ]
}
```

---

## 7. Guarantees

cpp-dep-bridge guarantees:

- deterministic output
- stable component identity
- no duplicate components
- conservative classification

---

## 8. Non-Goals

This model intentionally does **not**:

- guess versions without evidence
- infer licenses heuristically
- merge unrelated libraries
- rewrite build graphs

---

## 9. Status

Applies starting with **v0.3.0-alpha**.
Future releases may *extend* but not break this model.

