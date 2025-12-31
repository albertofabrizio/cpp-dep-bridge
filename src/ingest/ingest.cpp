#include "depbridge/ingest/ingest.hpp"
#include "depbridge/ingest/cmake/file_api_ingestor.hpp"

namespace depbridge::ingest
{

    model::ProjectGraph ingest(const std::filesystem::path &build_dir,
                               const IngestOptions &options)
    {
        return cmake::ingest_file_api(build_dir, options);
    }

}
