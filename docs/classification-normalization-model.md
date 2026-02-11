# Classification & Normalization Model (v0.4.0-alpha)

## Normalization

- Preserve raw link-token evidence (`cmake-link-token`) and path evidence (`cmake-link-path`).
- Imported targets normalize to package-level component names.
  - `fmt::fmt` -> component `fmt`
  - Original imported token is preserved as evidence (`cmake-imported`).
- Path-based normalization may strip platform naming conventions when path context exists.
- Ambiguous non-path tokens are not aggressively collapsed.

## Classification

Classification is evidence-gated and conservative.

Order:
1. `project_local`
2. `system`
3. `third_party`
4. `unknown`

Rules:
- `project_local`: explicit target-name match evidence.
- `system`: known system names + explicit link evidence.
- `third_party`: explicit package-manager/imported-target evidence.
- No explicit evidence => remain `unknown`.

This model prevents over-classification and avoids silent guessing.
