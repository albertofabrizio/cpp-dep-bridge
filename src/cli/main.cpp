#include "depbridge/ingest/ingest.hpp"
#include "depbridge/model/normalize.hpp"
#include "depbridge/model/classify.hpp"
#include "depbridge/model/filter.hpp"
#include "depbridge/model/variant_normalize.hpp"
#include "depbridge/sbom/cyclonedx_writer.hpp"
#include "depbridge/enrich/enrich.hpp"

#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    try
    {
        if (argc < 3 || std::string(argv[1]) != "scan")
        {
            std::cerr << "Usage: depbridge scan <build-dir> "
                         "[--include-system] [--include-project-local]\n";
            return 1;
        }

        depbridge::model::FilterOptions filter_opt;
        depbridge::enrich::EnrichmentConfig enrich_cfg;

        std::string build_dir;
        for (int i = 2; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--include-system")
                filter_opt.include_system = true;
            else if (arg == "--include-project-local")
                filter_opt.include_project_local = true;
            else if (arg == "--enrich")
                enrich_cfg.enabled = true;
            else if (arg == "--enrich-build-context")
                enrich_cfg.enable_build_context = true;
            else if (build_dir.empty())
                build_dir = arg;
            else
            {
                std::cerr << "Unknown argument: " << arg << "\n";
                return 1;
            }
        }

        if (enrich_cfg.enable_build_context && !enrich_cfg.enabled)
        {
            std::cerr << "--enrich-build-context requires --enrich\n";
            return 1;
        }

        if (build_dir.empty())
        {
            std::cerr << "Missing <build-dir>\n";
            return 1;
        }

        auto graph = depbridge::ingest::ingest(build_dir);
        depbridge::model::normalize_graph(graph);
        depbridge::model::normalize_build_variants(graph);
        depbridge::model::classify_project_local_components(graph);
        depbridge::model::classify_system_components(graph);
        depbridge::model::classify_third_party_components(graph);
        depbridge::model::filter_components(graph, filter_opt);
        auto enrichment = depbridge::enrich::enrich(graph, enrich_cfg);

        if (enrich_cfg.enabled)
        {
            depbridge::sbom::write_cyclonedx_json(std::cout, graph, enrichment);
        }
        else
        {
            depbridge::sbom::write_cyclonedx_json(std::cout, graph);
        }

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "depbridge error: " << e.what() << "\n";
        return 2;
    }
}
