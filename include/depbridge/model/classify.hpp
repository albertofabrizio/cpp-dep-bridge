#pragma once

#include "depbridge/model/types.hpp"

namespace depbridge::model
{
    struct ClassifyOptions
    {
        // Reserved for future policies; kept minimal for now.
    };

    void classify_project_local_components(ProjectGraph &g, const ClassifyOptions &opt = {});

    void classify_system_components(ProjectGraph &g);
}
