#include "depbridge/sbom/cyclonedx_writer.hpp"
#include "depbridge/model/ids.hpp"
#include "depbridge/model/types.hpp"
#include "depbridge/version.hpp"

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

    BuildTarget app_target;
    app_target.id = TargetId{"tgt:app"};
    app_target.name = "app";
    g.targets[app_target.id.value] = app_target;

    Component app;
    app.type = ComponentType::executable;
    app.name = "app";
    app.origin = ComponentOrigin::project_local;
    app.sources.push_back(SourceRef{"project-target", "app", std::nullopt});
    app.id = component_id_of(app);
    g.components[app.id.value] = app;

    Component fmt;
    fmt.type = ComponentType::library;
    fmt.name = "fmt";
    fmt.version = "10.2.1";
    fmt.purl = "pkg:github/fmtlib/fmt@10.2.1";
    fmt.license.spdx_id = "MIT";
    fmt.id = component_id_of(fmt);
    g.components[fmt.id.value] = fmt;

    DependencyEdge edge;
    edge.from = app_target.id;
    edge.to_component = fmt.id;
    g.edges.push_back(edge);

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

static void test_cyclonedx_contains_metadata_and_dependencies()
{
    auto g = make_graph();
    std::ostringstream os;
    depbridge::sbom::write_cyclonedx_json(os, g);
    const std::string s = os.str();

    assert(s.find("\"bomFormat\": \"CycloneDX\"") != std::string::npos);
    assert(s.find("\"version\": \"" + std::string(depbridge::version) + "\"") != std::string::npos);
    assert(s.find("\"metadata\"") != std::string::npos);
    assert(s.find("\"component\"") != std::string::npos);

    assert(s.find("\"dependencies\": [") != std::string::npos);

    std::string app_ref;
    std::string fmt_ref;
    for (const auto &[id, c] : g.components)
    {
        if (c.name == "app")
            app_ref = id;
        if (c.name == "fmt")
            fmt_ref = id;
    }

    assert(!app_ref.empty());
    assert(!fmt_ref.empty());

    const std::string expected_dependency =
        "\"ref\": \"" + app_ref + "\",\n      \"dependsOn\": [\"" + fmt_ref + "\"]";
    assert(s.find(expected_dependency) != std::string::npos);
}

int main()
{
    test_cyclonedx_is_deterministic();
    test_cyclonedx_contains_metadata_and_dependencies();

    std::cout << "[unit] cyclonedx: OK\n";
    return 0;
}
