#pragma once

#include "depbridge/model/types.hpp"
#include <string>

namespace depbridge::model
{
    std::string compute_fingerprint(const ProjectGraph &g);
}
