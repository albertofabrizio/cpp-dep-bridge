#pragma once

#include "depbridge/model/types.hpp"

#include <filesystem>

namespace depbridge::ingest
{

    struct IngestOptions
    {
        bool include_tests = false;
        bool include_toolchain_targets = false;
        bool include_generated_targets = false;
    };

    model::ProjectGraph ingest(const std::filesystem::path &build_dir,
                               const IngestOptions &options = {});

}
