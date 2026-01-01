#include "depbridge/model/filter.hpp"

#include <unordered_set>

namespace depbridge::model
{

    void filter_components(ProjectGraph &g, const FilterOptions &opt)
    {
        std::unordered_set<std::string> removed;

        for (const auto &[id, c] : g.components)
        {
            if (c.origin == ComponentOrigin::system && !opt.include_system)
            {
                removed.insert(id);
            }
            else if (c.origin == ComponentOrigin::project_local &&
                     !opt.include_project_local)
            {
                removed.insert(id);
            }
        }

        for (const auto &id : removed)
        {
            g.components.erase(id);
        }

        g.edges.erase(
            std::remove_if(
                g.edges.begin(),
                g.edges.end(),
                [&](const DependencyEdge &e)
                {
                    return e.to_component &&
                           removed.find(e.to_component->value) != removed.end();
                }),
            g.edges.end());
    }

}
