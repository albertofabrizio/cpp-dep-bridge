#include "depbridge/model/classify.hpp"
#include "depbridge/model/normalize.hpp"
#include "depbridge/model/ids.hpp"

#include <cassert>
#include <iostream>

using namespace depbridge::model;

static void test_system_classification()
{
    ProjectGraph g;

    Component sys;
    sys.name = "kernel32";
    sys.id = component_id_of(sys);
    g.components[sys.id.value] = sys;

    Component proj;
    proj.name = "depbridge_core";
    proj.origin = ComponentOrigin::project_local;
    proj.id = component_id_of(proj);
    g.components[proj.id.value] = proj;

    Component third;
    third.name = "fmt";
    third.id = component_id_of(third);
    g.components[third.id.value] = third;

    classify_system_components(g);

    assert(g.components[sys.id.value].origin == ComponentOrigin::system);
    assert(g.components[proj.id.value].origin == ComponentOrigin::project_local);
    assert(g.components[third.id.value].origin == ComponentOrigin::unknown);
}

int main()
{
    ProjectGraph g;

    // Simulate a project build target.
    BuildTarget t;
    t.id = TargetId{"t:depbridge_core"};
    t.name = "depbridge_core";
    g.targets.emplace(t.id.value, t);

    // Simulate that the linker token produced a component with same name.
    DependencyEdge e;
    e.from = t.id;
    e.raw = "depbridge_core";
    g.edges.push_back(e);

    normalize_graph(g);
    classify_project_local_components(g);

    // Find component by name.
    bool found = false;
    for (const auto &[_, c] : g.components)
    {
        if (c.name == "depbridge_core")
        {
            found = true;
            assert(c.origin == ComponentOrigin::project_local);
        }
    }
    assert(found);

    std::cout << "[unit] classify: OK\n";

    test_system_classification();

    return 0;
}
