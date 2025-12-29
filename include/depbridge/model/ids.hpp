#pragma once

#include "depbridge/model/types.hpp"

#include <string>
#include <string_view>

namespace depbridge::model
{

    std::string canonical_component_key(
        ComponentType type,
        std::string_view namespace_,
        std::string_view name,
        std::string_view version,
        std::string_view purl);

    std::string canonical_target_key(
        std::string_view buildsystem,
        std::string_view project,
        std::string_view target_name,
        std::string_view configuration);

    ComponentId make_component_id(
        ComponentType type,
        std::string_view namespace_,
        std::string_view name,
        std::string_view version,
        std::string_view purl = {});

    TargetId make_target_id(
        std::string_view buildsystem,
        std::string_view project,
        std::string_view target_name,
        std::string_view configuration = {});

    ComponentId component_id_of(const Component &c);

}
