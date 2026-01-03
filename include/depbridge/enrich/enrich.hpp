#pragma once

#include "depbridge/enrich/types.hpp"
#include "depbridge/model/types.hpp"

#include <vector>
#include <string>

namespace depbridge::enrich
{
    struct EnrichmentConfig
    {
        bool enabled{false};
        bool enable_build_context{false};
    };

    EnrichmentOverlay enrich(
        const depbridge::model::ProjectGraph &graph,
        const EnrichmentConfig &cfg);
}
