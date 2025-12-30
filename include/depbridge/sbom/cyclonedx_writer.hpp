#pragma once

#include "depbridge/model/types.hpp"

#include <ostream>

namespace depbridge::sbom
{
    void write_cyclonedx_json(std::ostream &os, const depbridge::model::ProjectGraph &g);

}