# Architecture

## Phase contracts (v0.4.0-alpha)

The pipeline is intentionally layered. Each phase consumes typed model data and emits explicit evidence.

1. **Ingestion (CMake File API)**
   - Inputs: CMake File API reply JSON files.
   - Outputs: `ProjectGraph.targets`, raw `DependencyEdge.raw`, optional `DependencyEdge.to_target`, and `SourceRef` evidence.
   - Contract: ingestion records raw build facts only. No package-name guessing from paths in default mode.

2. **Normalization**
   - Inputs: raw edge tokens + target references.
   - Outputs: `Component`s, `DependencyEdge.to_component`, normalized names, preserved raw evidence.
   - Contract: imported targets normalize to package-level identities (e.g., `fmt::fmt` -> `fmt`) while preserving original imported token evidence.

3. **Classification**
   - Inputs: components + evidence.
   - Outputs: `Component.origin`.
   - Contract: evidence-gated classification only. Insufficient evidence remains `unknown`.

4. **Filtering**
   - Inputs: classified graph.
   - Outputs: pruned graph by policy.

5. **SBOM emission**
   - Inputs: filtered graph.
   - Outputs: deterministic CycloneDX JSON components + dependencies + metadata.

## Evidence contract

`SourceRef` remains the transport object between phases. Classification and policy logic are keyed through `EvidenceKind` adapters, not ad hoc free-form string checks.

## Breaking behavior changes in v0.4.0-alpha

- Imported target component identities are package-level.
- Default pipeline removes ingestion-time package-manager guessing.
- Classification is conservative and defaults to `unknown` unless explicit evidence exists.
- CycloneDX output includes deterministic `dependencies` and metadata subject when resolvable.
