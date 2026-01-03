#pragma once

#include "depbridge/enrich/provider.hpp"

namespace depbridge::enrich::providers
{
    class BuildContextProvider final : public EnrichmentProvider
    {
    public:
        std::string name() const override { return "build-context"; }
        std::string version() const override { return "0.4.0-alpha"; }

        void enrich(
            const depbridge::model::ProjectGraph &graph,
            const EnrichmentConfig &cfg,
            EnrichmentOverlay &out) const override;
    };
}
