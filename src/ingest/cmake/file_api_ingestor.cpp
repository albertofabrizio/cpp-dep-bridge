#include "depbridge/ingest/cmake/file_api_ingestor.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace depbridge::ingest::cmake
{

    using json = nlohmann::json;
    using namespace depbridge::model;
    namespace fs = std::filesystem;

    static json load_json(const fs::path &p)
    {
        std::ifstream f(p);
        if (!f)
        {
            throw std::runtime_error("Failed to open JSON file: " + p.string());
        }
        json j;
        f >> j;
        return j;
    }

    static fs::path find_reply_dir(const fs::path &build_dir)
    {
        fs::path reply = build_dir / ".cmake" / "api" / "v1" / "reply";
        if (!fs::exists(reply))
        {
            throw std::runtime_error(
                "CMake File API reply directory not found: " + reply.string());
        }
        return reply;
    }

    static fs::path find_index(const fs::path &reply_dir)
    {
        for (const auto &e : fs::directory_iterator(reply_dir))
        {
            const auto name = e.path().filename().string();
            if (name.rfind("index-", 0) == 0 && e.path().extension() == ".json")
            {
                return e.path();
            }
        }
        throw std::runtime_error("CMake File API index-*.json not found");
    }

    ProjectGraph ingest_file_api(const fs::path &build_dir,
                                 const IngestOptions &)
    {
        ProjectGraph g;

        g.context.run_id = "cmake-file-api";
        g.context.root_directory = build_dir.string();
        g.context.build_directory = build_dir.string();

        const fs::path reply_dir = find_reply_dir(build_dir);
        const fs::path index_path = find_index(reply_dir);
        const json index = load_json(index_path);

        fs::path codemodel_path;
        for (const auto &obj : index.at("objects"))
        {
            if (obj.at("kind") == "codemodel" &&
                obj.at("version").at("major") == 2)
            {
                codemodel_path = reply_dir / obj.at("jsonFile").get<std::string>();
                break;
            }
        }

        if (codemodel_path.empty())
        {
            throw std::runtime_error("codemodel-v2 not found in File API index");
        }

        const json codemodel = load_json(codemodel_path);

        for (const auto &cfg : codemodel.at("configurations"))
        {
            const std::string cfg_name = cfg.at("name").get<std::string>();

            for (const auto &tgt_ref : cfg.at("targets"))
            {
                const fs::path tgt_path =
                    reply_dir / tgt_ref.at("jsonFile").get<std::string>();
                const json tgt = load_json(tgt_path);

                BuildTarget bt;
                bt.name = tgt.at("name").get<std::string>();
                bt.id = TargetId{"raw:" + bt.name + ":" + cfg_name};

                bt.sources.push_back(SourceRef{
                    "cmake",
                    "target/" + bt.name,
                    std::nullopt});

                g.targets.emplace(bt.id.value, bt);

                if (tgt.contains("link"))
                {
                    const auto &link = tgt.at("link");
                    if (link.contains("libraries"))
                    {
                        for (const auto &lib : link.at("libraries"))
                        {
                            DependencyEdge e;
                            e.from = bt.id;
                            e.raw = lib.get<std::string>();

                            e.sources.push_back(SourceRef{
                                "cmake",
                                "linkLibraries",
                                std::nullopt});

                            g.edges.push_back(e);
                        }
                    }
                }
            }
        }

        return g;
    }

}
