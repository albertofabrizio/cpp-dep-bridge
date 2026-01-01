#include "depbridge/model/filter.hpp"
#include "depbridge/model/ids.hpp"

#include <cassert>

using namespace depbridge::model;

static void test_filter_default_behavior()
{
    ProjectGraph g;

    Component sys;
    sys.name = "kernel32";
    sys.origin = ComponentOrigin::system;
    sys.id = component_id_of(sys);
    g.components[sys.id.value] = sys;

    Component proj;
    proj.name = "depbridge_core";
    proj.origin = ComponentOrigin::project_local;
    proj.id = component_id_of(proj);
    g.components[proj.id.value] = proj;

    Component third;
    third.name = "fmt";
    third.origin = ComponentOrigin::unknown;
    third.id = component_id_of(third);
    g.components[third.id.value] = third;

    FilterOptions opt;
    filter_components(g, opt);

    assert(g.components.find(sys.id.value) == g.components.end());
    assert(g.components.find(proj.id.value) == g.components.end());
    assert(g.components.find(third.id.value) != g.components.end());
}

static void test_filter_include_all()
{
    ProjectGraph g;

    Component sys;
    sys.name = "kernel32";
    sys.origin = ComponentOrigin::system;
    sys.id = component_id_of(sys);
    g.components[sys.id.value] = sys;

    FilterOptions opt;
    opt.include_system = true;

    filter_components(g, opt);

    assert(g.components.find(sys.id.value) != g.components.end());
}

int main()
{
    test_filter_default_behavior();
    test_filter_include_all();
    return 0;
}
