#include "depbridge/model/classify.hpp"
#include "depbridge/model/normalize.hpp"
#include "depbridge/model/ids.hpp"

#include <cassert>
#include <iostream>

using namespace depbridge::model;

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
    for (const auto& [_, c] : g.components)
    {
        if (c.name == "depbridge_core")
        {
            found = true;
            assert(c.origin == ComponentOrigin::project_local);
        }
    }
    assert(found);

    std::cout << "[unit] classify: OK\n";
    return 0;
}
