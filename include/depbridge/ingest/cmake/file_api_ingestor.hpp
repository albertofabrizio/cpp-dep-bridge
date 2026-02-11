#pragma once

#include "depbridge/ingest/ingest.hpp"

#include <vector>

namespace depbridge::ingest::cmake
{

    std::filesystem::path select_deterministic_file_api_index(const std::vector<std::filesystem::path> &candidates);

    model::ProjectGraph ingest_file_api(const std::filesystem::path &build_dir,
                                        const IngestOptions &options);

}
