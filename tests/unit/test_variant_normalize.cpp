#include "depbridge/model/variant_normalize.hpp"
#include "depbridge/model/ids.hpp"

#include <cassert>
#include <iostream>

using namespace depbridge::model;

static ProjectGraph make_graph()
{
    ProjectGraph g;

    // Debug vcpkg artifact: fmtd.lib should collapse into fmt
    Component a;
    a.type = ComponentType::library;
    a.origin = ComponentOrigin::third_party;
    a.name = "fmtd";
    a.sources.push_back(SourceRef{"link-token", "D:/vcpkg/installed/x64-windows/debug/lib/fmtd.lib", std::nullopt});
    a.id = component_id_of(a);
    g.components[a.id.value] = a;

    // Release logical fmt component
    Component b;
    b.type = ComponentType::library;
    b.origin = ComponentOrigin::third_party;
    b.name = "fmt";
    b.sources.push_back(SourceRef{"package-manager", "vcpkg", std::nullopt});
    b.id = component_id_of(b);
    g.components[b.id.value] = b;

    // Edge points to fmtd initially -> must remap to fmt
    DependencyEdge e;
    e.from = TargetId{"t:app"};
    e.to_component = a.id;
    g.edges.push_back(e);

    return g;
}

static void test_variant_normalize_merges_fmtd_into_fmt()
{
    auto g = make_graph();

    normalize_build_variants(g);

    bool has_fmt = false;
    bool has_fmtd = false;
    std::string fmt_id;

    for (const auto &[id, c] : g.components)
    {
        if (c.name == "fmt")
        {
            has_fmt = true;
            fmt_id = id;

            // Evidence should be preserved in properties
            auto itV = c.properties.find("depbridge:variant");
            auto itE = c.properties.find("depbridge:variant.evidence");
            auto itO = c.properties.find("depbridge:variant.original-name");

            assert(itV != c.properties.end());
            assert(itE != c.properties.end());
            assert(itO != c.properties.end());

            // debug evidence (path + suffix)
            assert(itV->second.find("debug") != std::string::npos);
            assert(itE->second.find("path:/debug/") != std::string::npos);
            assert(itE->second.find("msvc-suffix-d") != std::string::npos);
            assert(itO->second.find("fmtd") != std::string::npos);

            // Origin preserved
            assert(c.origin == ComponentOrigin::third_party);
        }
        if (c.name == "fmtd")
            has_fmtd = true;
    }

    assert(has_fmt);
    assert(!has_fmtd);

    // Edge remapped to canonical fmt component id
    assert(!g.edges.empty());
    assert(g.edges[0].to_component.has_value());
    assert(g.edges[0].to_component->value == fmt_id);
}

int main()
{
    test_variant_normalize_merges_fmtd_into_fmt();
    std::cout << "[unit] variant_normalize: OK\n";
    return 0;
}
