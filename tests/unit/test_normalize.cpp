#include "depbridge/model/normalize.hpp"
#include "depbridge/model/ids.hpp"

#include <cassert>
#include <iostream>

using namespace depbridge::model;

static void test_token_to_component_unix_dash_l()
{
    NormalizeOptions opt;
    auto c = component_from_link_token("-lssl", opt);
    assert(c.type == ComponentType::library);
    assert(c.name == "ssl");
    assert(!c.version);
    assert(!c.purl);
    assert(!c.namespace_);
}

static void test_token_to_component_windows_lib_path()
{
    NormalizeOptions opt;
    auto c = component_from_link_token("D:\\libs\\FMT.LIB", opt);
    assert(c.type == ComponentType::library);
    // case folding is enabled by default
    assert(c.name == "fmt");
}

static void test_token_to_component_cmake_target_passthrough()
{
    NormalizeOptions opt;
    auto c = component_from_link_token("OpenSSL::SSL", opt);
    assert(c.name == "OpenSSL::SSL");
    assert(c.type == ComponentType::library);
}

static void test_merge_component_fills_missing()
{
    Component dst;
    dst.id = make_component_id(ComponentType::library, "", "fmt", "", "");
    dst.type = ComponentType::library;
    dst.name = "fmt";

    Component inc;
    inc.type = ComponentType::library;
    inc.name = "fmt";
    inc.version = "10.2.1";
    inc.purl = "pkg:github/fmtlib/fmt@10.2.1";
    inc.license.spdx_id = "MIT";
    inc.properties["k1"] = "v1";

    merge_component(dst, inc);

    assert(dst.name == "fmt");
    assert(dst.version && *dst.version == "10.2.1");
    assert(dst.purl && *dst.purl == "pkg:github/fmtlib/fmt@10.2.1");
    assert(dst.license.spdx_id && *dst.license.spdx_id == "MIT");
    assert(dst.properties["k1"] == "v1");
}

static void test_normalize_graph_merges_duplicates_and_remaps_edges()
{
    ProjectGraph g;
    g.context.run_id = "test";
    g.context.root_directory = "root";
    g.context.build_directory = "build";

    Component c1;
    c1.type = ComponentType::library;
    c1.name = "fmt";
    c1.purl = "pkg:github/fmtlib/fmt@10.2.1";
    c1.id = ComponentId{"old1"};
    g.components["old1"] = c1;

    Component c2 = c1;
    c2.name = "FMT"; // different name shouldn't matter with purl
    c2.id = ComponentId{"old2"};
    g.components["old2"] = c2;

    DependencyEdge e;
    e.from = TargetId{"tgt_x"};
    e.to_component = ComponentId{"old2"};
    g.edges.push_back(e);

    normalize_graph(g);

    assert(g.components.size() == 1);

    const auto &only = g.components.begin()->second;
    assert(only.id.value.rfind("cmp_", 0) == 0); // starts with cmp_
    assert(g.edges.size() == 1);
    assert(g.edges[0].to_component);
    assert(g.edges[0].to_component->value == only.id.value);
}

static void test_imported_cmake_targets_create_components()
{
    ProjectGraph g;

    DependencyEdge e1;
    e1.raw = std::string("fmt::fmt");

    DependencyEdge e2;
    e2.raw = std::string("nlohmann_json::nlohmann_json");

    g.edges.push_back(e1);
    g.edges.push_back(e2);

    NormalizeOptions opt;
    normalize_graph(g, opt);

    // We should get one component per imported target (no namespace collapsing).
    assert(g.components.size() == 2);

    {
        Component c;
        c.type = ComponentType::library;
        c.name = "fmt::fmt";
        const ComponentId expected = component_id_of(c);
        assert(g.components.count(expected.value) == 1);
    }

    {
        Component c;
        c.type = ComponentType::library;
        c.name = "nlohmann_json::nlohmann_json";
        const ComponentId expected = component_id_of(c);
        assert(g.components.count(expected.value) == 1);
    }

    // Edges must have been remapped to component IDs.
    assert(g.edges[0].to_component.has_value());
    assert(g.edges[1].to_component.has_value());

    assert(g.components.count(g.edges[0].to_component->value) == 1);
    assert(g.components.count(g.edges[1].to_component->value) == 1);
}

int main()
{
    test_token_to_component_unix_dash_l();
    test_token_to_component_windows_lib_path();
    test_token_to_component_cmake_target_passthrough();
    test_merge_component_fills_missing();
    test_normalize_graph_merges_duplicates_and_remaps_edges();
    test_imported_cmake_targets_create_components();

    std::cout << "[unit] normalize: OK\n";
    return 0;
}
