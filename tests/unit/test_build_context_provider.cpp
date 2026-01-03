#include "depbridge/enrich/enrich.hpp"
#include "depbridge/model/types.hpp"

#include <cassert>

using namespace depbridge;

int main()
{
    model::ProjectGraph g;

    // Component used by targets
    model::Component c;
    c.id = model::ComponentId{"compA"};
    c.name = "libA";
    g.components.emplace(c.id.value, c);

    // Two consuming targets with different build context
    model::BuildTarget t1;
    t1.id = model::TargetId{"t1"};
    t1.name = "app1";
    t1.configuration = "Debug";
    t1.toolchain = "gcc";
    t1.platform = "linux";
    g.targets.emplace(t1.id.value, t1);

    model::BuildTarget t2;
    t2.id = model::TargetId{"t2"};
    t2.name = "app2";
    t2.configuration = "Release";
    t2.toolchain = "gcc";
    t2.platform = "linux";
    g.targets.emplace(t2.id.value, t2);

    // Edges: t1 -> compA, t2 -> compA
    model::DependencyEdge e1;
    e1.from = t1.id;
    e1.to_component = c.id;
    g.edges.push_back(e1);

    model::DependencyEdge e2;
    e2.from = t2.id;
    e2.to_component = c.id;
    g.edges.push_back(e2);

    enrich::EnrichmentConfig cfg;
    cfg.enabled = true;
    cfg.enable_build_context = true;

    auto o = enrich::enrich(g, cfg);

    assert(!o.model_fingerprint.empty());
    assert(!o.overlay_hash.empty());

    auto it = o.component_fields.find("compA");
    assert(it != o.component_fields.end());

    const auto &fields = it->second;

    // Must include used_by_targets_count
    bool has_count = false;
    bool has_cfgs = false;

    for (const auto &f : fields)
    {
        if (f.key == "depbridge:enrich.build.used_by_targets_count")
        {
            has_count = true;
            assert(std::get<int64_t>(f.value) == 2);
            assert(f.provenance.provider == "build-context");
            assert(f.provenance.rule_id == "AGG_FROM_CONSUMING_TARGETS");
        }
        else if (f.key == "depbridge:enrich.build.configurations")
        {
            has_cfgs = true;
            const auto &v = std::get<std::vector<std::string>>(f.value);
            assert(v.size() == 2);
            assert(v[0] == "Debug");
            assert(v[1] == "Release");
        }
    }

    assert(has_count);
    assert(has_cfgs);

    return 0;
}
