#pragma once

#include "depbridge/enrich/types.hpp"
#include "depbridge/model/types.hpp"

namespace depbridge::enrich
{
    struct EnrichmentConfig
    {
        bool enabled{false}; // future use
    };

    EnrichmentOverlay enrich(
        const depbridge::model::ProjectGraph &graph,
        const EnrichmentConfig &cfg);
}
