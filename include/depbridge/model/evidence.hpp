#pragma once

#include "depbridge/model/types.hpp"

namespace depbridge::model
{
    enum class EvidenceKind
    {
        unknown,
        cmake_link_token,
        cmake_link_path,
        cmake_target,
        cmake_target_id,
        cmake_imported_target,
        package_manager,
        package_manager_path,
        system_library_name,
        project_target_match
    };

    EvidenceKind evidence_kind_from_source(const SourceRef &source);
}
