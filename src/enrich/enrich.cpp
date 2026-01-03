#include "depbridge/enrich/enrich.hpp"
#include "depbridge/model/fingerprint.hpp"

namespace depbridge::enrich
{
    EnrichmentOverlay enrich(
        const depbridge::model::ProjectGraph &graph,
        const EnrichmentConfig & /*cfg*/
    )
    {
        EnrichmentOverlay out;
        out.model_fingerprint = depbridge::model::compute_fingerprint(graph);
        return out; // No-op enrichment
    }
}
