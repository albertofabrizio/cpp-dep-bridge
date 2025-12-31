#pragma once

#include "depbridge/ingest/ingest.hpp"

namespace depbridge::ingest::cmake
{

    model::ProjectGraph ingest_file_api(const std::filesystem::path &build_dir,
                                        const IngestOptions &options);

}
