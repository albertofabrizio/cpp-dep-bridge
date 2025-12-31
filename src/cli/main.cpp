#include "depbridge/ingest/ingest.hpp"
#include "depbridge/model/normalize.hpp"
#include "depbridge/model/classify.hpp"
#include "depbridge/sbom/cyclonedx_writer.hpp"

#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    try
    {
        if (argc != 3 || std::string(argv[1]) != "scan")
        {
            std::cerr << "Usage: depbridge scan <build-dir>\n";
            return 1;
        }

        const std::string build_dir = argv[2];

        auto graph = depbridge::ingest::ingest(build_dir);
        depbridge::model::normalize_graph(graph);
        depbridge::model::classify_project_local_components(graph);
        depbridge::sbom::write_cyclonedx_json(std::cout, graph);

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "depbridge error: " << e.what() << "\n";
        return 2;
    }
}
