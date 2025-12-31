#include "depbridge/model/classify.hpp"

#include <unordered_set>

namespace depbridge::model
{
    void classify_project_local_components(ProjectGraph &g, const ClassifyOptions &)
    {
        std::unordered_set<std::string> target_names;
        target_names.reserve(g.targets.size());
        for (const auto &[_, t] : g.targets)
        {
            if (!t.name.empty())
                target_names.insert(t.name);
        }

        for (auto &[_, c] : g.components)
        {
            if (c.origin != ComponentOrigin::unknown)
                continue;
            if (target_names.find(c.name) != target_names.end())
            {
                c.origin = ComponentOrigin::project_local;
            }
        }
    }
}
