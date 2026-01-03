#include "depbridge/enrich/enrich.hpp"
#include "depbridge/model/types.hpp"

#include <cassert>

using namespace depbridge;

int main()
{
    model::ProjectGraph g;
    g.components.emplace(
        "comp1",
        model::Component{
            model::ComponentId{"comp1"},
            "example-lib"});

    enrich::EnrichmentConfig cfg;

    auto a = enrich::enrich(g, cfg);
    auto b = enrich::enrich(g, cfg);

    assert(a.model_fingerprint == b.model_fingerprint);
    assert(a.component_fields.empty());
    assert(b.component_fields.empty());

    return 0;
}
