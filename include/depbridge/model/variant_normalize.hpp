#pragma once

#include "depbridge/model/types.hpp"

namespace depbridge::model
{
    struct VariantNormalizeOptions
    {
        bool enable = true;
        bool msvc_strip_debug_suffix_d = true;
        bool infer_debug_from_paths = true;
        bool record_evidence_in_properties = true;
    };

    void normalize_build_variants(ProjectGraph &g, const VariantNormalizeOptions &opt = {});
}
