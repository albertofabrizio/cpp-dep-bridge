#pragma once

#include "depbridge/enrich/types.hpp"
#include "depbridge/enrich/enrich.hpp"
#include "depbridge/model/types.hpp"

#include <string>

namespace depbridge::enrich
{
    class EnrichmentProvider
    {
    public:
        virtual ~EnrichmentProvider() = default;

        virtual std::string name() const = 0;
        virtual std::string version() const = 0;

        virtual void enrich(
            const depbridge::model::ProjectGraph &graph,
            const EnrichmentConfig &cfg,
            EnrichmentOverlay &out) const = 0;
    };
}
