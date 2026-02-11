#include "depbridge/model/evidence.hpp"

#include <string_view>

namespace depbridge::model
{
    EvidenceKind evidence_kind_from_source(const SourceRef &source)
    {
        if (source.system == "cmake-link-token")
            return EvidenceKind::cmake_link_token;
        if (source.system == "cmake-link-path")
            return EvidenceKind::cmake_link_path;
        if (source.system == "cmake")
            return EvidenceKind::cmake_target;
        if (source.system == "cmake-target-id")
            return EvidenceKind::cmake_target_id;
        if (source.system == "cmake-imported")
            return EvidenceKind::cmake_imported_target;
        if (source.system == "system-lib")
            return EvidenceKind::system_library_name;
        if (source.system == "project-target")
            return EvidenceKind::project_target_match;

        if (source.system == "vcpkg" || source.system == "conan")
            return EvidenceKind::package_manager;
        if (source.system == "vcpkg-path" || source.system == "conan-path")
            return EvidenceKind::package_manager_path;

        return EvidenceKind::unknown;
    }
}
