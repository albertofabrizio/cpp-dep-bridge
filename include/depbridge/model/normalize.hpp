#pragma once

#include "depbridge/model/types.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace depbridge::model
{

    struct NormalizeOptions
    {
        bool case_fold_windows_libs = true;
        bool strip_unix_lib_prefix = true;
        bool strip_library_extensions = true;
    };

    std::string normalize_path(std::string_view path);

    std::string normalize_token(std::string_view token);

    Component component_from_link_token(std::string_view raw_token, const NormalizeOptions &opt = {});

    void merge_component(Component &dst, const Component &incoming);

    void normalize_graph(ProjectGraph &g, const NormalizeOptions &opt = {});

}
