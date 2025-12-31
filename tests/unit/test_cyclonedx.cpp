#include "depbridge/sbom/cyclonedx_writer.hpp"
#include "depbridge/model/ids.hpp"
#include "depbridge/model/types.hpp"

#include <cassert>
#include <iostream>
#include <sstream>

using namespace depbridge::model;

static ProjectGraph make_graph()
{
    ProjectGraph g;
    g.context.run_id = "run";
    g.context.root_directory = "root";
    g.context.build_directory = "build";

    Component a;
    a.type = ComponentType::library;
    a.name = "fmt";
    a.version = "10.2.1";
    a.purl = "pkg:github/fmtlib/fmt@10.2.1";
    a.license.spdx_id = "MIT";
    a.id = component_id_of(a);
    g.components[a.id.value] = a;

    Component b;
    b.type = ComponentType::unknown;
    b.name = "OpenSSL::SSL";
    b.id = component_id_of(b);
    g.components[b.id.value] = b;

    return g;
}

static void test_cyclonedx_is_deterministic()
{
    auto g = make_graph();
    std::ostringstream o1, o2;
    depbridge::sbom::write_cyclonedx_json(o1, g);
    depbridge::sbom::write_cyclonedx_json(o2, g);
    assert(o1.str() == o2.str());
}

static void test_cyclonedx_contains_components_and_fields()
{
    auto g = make_graph();
    std::ostringstream os;
    depbridge::sbom::write_cyclonedx_json(os, g);
    const std::string s = os.str();

    // Required top-level fields
    assert(s.find("\"bomFormat\": \"CycloneDX\"") != std::string::npos);
    assert(s.find("\"specVersion\": \"1.5\"") != std::string::npos);
    assert(s.find("\"components\": [") != std::string::npos);

    // fmt component fields
    assert(s.find("\"name\": \"fmt\"") != std::string::npos);
    assert(s.find("\"version\": \"10.2.1\"") != std::string::npos);
    assert(s.find("\"purl\": \"pkg:github/fmtlib/fmt@10.2.1\"") != std::string::npos);
    assert(s.find("\"licenses\":") != std::string::npos);
    assert(s.find("\"id\": \"MIT\"") != std::string::npos);

    // OpenSSL::SSL present (unknown type maps to library in MVP)
    assert(s.find("\"name\": \"OpenSSL::SSL\"") != std::string::npos);

    assert(s.find("\"name\": \"depbridge:origin\"") != std::string::npos);
    assert(s.find("\"value\": \"unknown\"") != std::string::npos);
}

static void test_cyclonedx_components_sorted_by_id()
{
    auto g = make_graph();
    std::ostringstream os;
    depbridge::sbom::write_cyclonedx_json(os, g);
    const std::string s = os.str();

    // Components are sorted by ComponentId (bom-ref)
    auto it = g.components.begin();
    std::string first_id = it->first;
    ++it;
    std::string second_id = it->first;

    assert(first_id < second_id);

    const auto p1 = s.find(first_id);
    const auto p2 = s.find(second_id);
    assert(p1 != std::string::npos && p2 != std::string::npos);
    assert(p1 < p2);
}

int main()
{
    test_cyclonedx_is_deterministic();
    test_cyclonedx_contains_components_and_fields();
    test_cyclonedx_components_sorted_by_id();

    std::cout << "[unit] cyclonedx: OK\n";
    return 0;
}
