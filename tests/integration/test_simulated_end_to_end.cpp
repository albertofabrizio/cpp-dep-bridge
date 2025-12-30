#include "depbridge/sbom/cyclonedx_writer.hpp"
#include "depbridge/model/normalize.hpp"
#include "depbridge/model/ids.hpp"
#include "depbridge/model/types.hpp"

#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace depbridge::model;

// NOTE:
// This integration test does NOT use the real CMake File API yet.
// Instead, it simulates what the ingestion layer will produce,
// ensuring Phase 3–5 integrate correctly end-to-end.

static ProjectGraph make_simulated_graph() {
    ProjectGraph g;
    g.context.run_id = "integration";
    g.context.root_directory = "fixture";
    g.context.build_directory = "build";

    // Simulate a target linking to "-lssl" and "fmt.lib"
    Component c1 = component_from_link_token("-lssl");
    Component c2 = component_from_link_token("FMT.LIB");

    g.components[c1.id.value] = c1;
    g.components[c2.id.value] = c2;

    DependencyEdge e;
    e.from = TargetId{"tgt_app"};
    e.to_component = c1.id;
    g.edges.push_back(e);

    return g;
}

static void test_end_to_end_sbom_generation() {
    auto g = make_simulated_graph();

    normalize_graph(g);

    std::ostringstream os;
    depbridge::sbom::write_cyclonedx_json(os, g);
    const std::string sbom = os.str();

    // High-level assertions
    assert(sbom.find("\"bomFormat\": \"CycloneDX\"") != std::string::npos);
    assert(sbom.find("\"components\": [") != std::string::npos);

    // Normalized component names
    assert(sbom.find("\"name\": \"ssl\"") != std::string::npos);
    assert(sbom.find("\"name\": \"fmt\"") != std::string::npos);

    // Deterministic ID prefixes
    assert(sbom.find("\"bom-ref\": \"cmp_") != std::string::npos);
}

int main() {
    test_end_to_end_sbom_generation();
    std::cout << "[integration] simulated end-to-end: OK\n";
    return 0;
}
