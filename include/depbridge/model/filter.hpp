#pragma once

#include "depbridge/model/types.hpp"

namespace depbridge::model
{

    struct FilterOptions
    {
        bool include_system = false;
        bool include_project_local = false;
    };

    void filter_components(ProjectGraph &g, const FilterOptions &opt);

}
