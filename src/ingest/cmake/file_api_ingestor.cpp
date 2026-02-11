#include "depbridge/ingest/cmake/file_api_ingestor.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

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

        try
        {
            json j;
            f >> j;
            return j;
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("Failed to parse JSON file '" + p.string() + "': " + e.what());
        }
    }

    static fs::path find_reply_dir(const fs::path &build_dir)
    {
        fs::path reply = build_dir / ".cmake" / "api" / "v1" / "reply";
        if (!fs::exists(reply))
        {
            throw std::runtime_error("CMake File API reply directory not found: " + reply.string());
        }
        if (!fs::is_directory(reply))
        {
            throw std::runtime_error("CMake File API reply path is not a directory: " + reply.string());
        }
        return reply;
    }

    fs::path select_deterministic_file_api_index(const std::vector<fs::path> &candidates)
    {
        if (candidates.empty())
        {
            throw std::runtime_error("No index file candidates provided");
        }

        std::vector<std::string> names;
        names.reserve(candidates.size());
        for (const auto &candidate : candidates)
        {
            names.push_back(candidate.filename().string());
        }

        std::sort(names.begin(), names.end());
        return candidates.front().parent_path() / names.back();
    }

    static fs::path find_index(const fs::path &reply_dir)
    {
        std::vector<fs::path> candidates;
        for (const auto &e : fs::directory_iterator(reply_dir))
        {
            if (!e.is_regular_file())
                continue;

            const auto name = e.path().filename().string();
            if (name.rfind("index-", 0) == 0 && e.path().extension() == ".json")
            {
                candidates.push_back(e.path());
            }
        }

        if (candidates.empty())
        {
            throw std::runtime_error("CMake File API index-*.json not found in reply directory: " + reply_dir.string());
        }

        return select_deterministic_file_api_index(candidates);
    }

    static bool is_noise_token(const std::string &s)
    {
        if (s.empty())
            return true;

        if (s.find("$<") != std::string::npos)
            return true;

        if (s.rfind("-Wl,", 0) == 0)
            return true;

        return false;
    }

    static std::vector<std::string> split_ws(const std::string &s)
    {
        std::vector<std::string> out;
        std::string cur;
        bool in_quotes = false;
        bool escaped = false;

        for (char ch : s)
        {
            if (escaped)
            {
                cur.push_back(ch);
                escaped = false;
                continue;
            }

            if (in_quotes && ch == '\\')
            {
                escaped = true;
                continue;
            }

            if (ch == '"')
            {
                in_quotes = !in_quotes;
                continue;
            }

            if (std::isspace(static_cast<unsigned char>(ch)) && !in_quotes)
            {
                if (!cur.empty())
                {
                    out.push_back(cur);
                    cur.clear();
                }
                continue;
            }

            cur.push_back(ch);
        }

        if (!cur.empty())
            out.push_back(cur);

        return out;
    }

    static std::string make_cfg_key(const std::string &cfg_name, const std::string &token)
    {
        return cfg_name + "\n" + token;
    }

    static std::optional<TargetId> resolve_target_token(
        const std::string &cfg_name,
        const std::string &token,
        const std::unordered_map<std::string, TargetId> &cmake_id_to_target,
        const std::unordered_map<std::string, TargetId> &cmake_name_to_target)
    {
        auto by_id = cmake_id_to_target.find(make_cfg_key(cfg_name, token));
        if (by_id != cmake_id_to_target.end())
            return by_id->second;

        auto by_name = cmake_name_to_target.find(make_cfg_key(cfg_name, token));
        if (by_name != cmake_name_to_target.end())
            return by_name->second;

        return std::nullopt;
    }

    static void push_edge(ProjectGraph &g,
                          const TargetId &from,
                          const std::string &cfg_name,
                          const std::string &raw,
                          const std::string &ref,
                          const std::unordered_map<std::string, TargetId> &cmake_id_to_target,
                          const std::unordered_map<std::string, TargetId> &cmake_name_to_target)
    {
        if (is_noise_token(raw))
            return;

        DependencyEdge e;
        e.from = from;
        e.raw = raw;
        e.to_target = resolve_target_token(cfg_name, raw, cmake_id_to_target, cmake_name_to_target);

        e.sources.push_back(SourceRef{"cmake", ref, std::nullopt});

        g.edges.push_back(std::move(e));
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

        std::unordered_map<std::string, TargetId> cmake_id_to_target;
        std::unordered_map<std::string, TargetId> cmake_name_to_target;

        try
        {
            for (const auto &cfg : codemodel.at("configurations"))
            {
                const std::string cfg_name = cfg.at("name").get<std::string>();

                for (const auto &tgt_ref : cfg.at("targets"))
                {
                    const fs::path tgt_path = reply_dir / tgt_ref.at("jsonFile").get<std::string>();

                    try
                    {
                        const json tgt = load_json(tgt_path);

                        BuildTarget bt;
                        bt.name = tgt.at("name").get<std::string>();
                        bt.id = TargetId{"raw:" + bt.name + ":" + cfg_name};
                        bt.configuration = cfg_name;

                        const std::string cmake_tid = tgt.contains("id") && tgt.at("id").is_string()
                                                          ? tgt.at("id").get<std::string>()
                                                          : std::string{};

                        bt.sources.push_back(SourceRef{"cmake", "target/" + bt.name, std::nullopt});

                        if (!cmake_tid.empty())
                        {
                            bt.sources.push_back(SourceRef{"cmake-target-id", cmake_tid, std::nullopt});

                            const auto [_, inserted] = cmake_id_to_target.emplace(make_cfg_key(cfg_name, cmake_tid), bt.id);
                            if (!inserted)
                            {
                                throw std::runtime_error("Duplicate CMake target id mapping: '" + cmake_tid +
                                                         "' in configuration '" + cfg_name + "'");
                            }
                        }

                        const auto [__, inserted_name] = cmake_name_to_target.emplace(make_cfg_key(cfg_name, bt.name), bt.id);
                        if (!inserted_name)
                        {
                            throw std::runtime_error("Duplicate target name mapping: '" + bt.name +
                                                     "' in configuration '" + cfg_name + "'");
                        }

                        const bool is_generator = tgt.contains("isGeneratorProvided") &&
                                                  tgt.at("isGeneratorProvided").is_boolean() &&
                                                  tgt.at("isGeneratorProvided").get<bool>();
                        const bool has_artifacts = tgt.contains("artifacts");

                        if (!is_generator && !has_artifacts)
                        {
                            bt.sources.push_back(SourceRef{"cmake-imported", bt.name, std::nullopt});
                        }

                        const auto [___, inserted_target] = g.targets.emplace(bt.id.value, bt);
                        if (!inserted_target)
                        {
                            throw std::runtime_error("Duplicate build target id insertion: '" + bt.id.value +
                                                     "' in configuration '" + cfg_name + "'");
                        }

                        if (!tgt.contains("link"))
                            continue;

                        const auto &link = tgt.at("link");

                        if (link.contains("libraries") && link.at("libraries").is_array())
                        {
                            for (const auto &lib : link.at("libraries"))
                            {
                                if (lib.is_string())
                                {
                                    push_edge(g, bt.id, cfg_name, lib.get<std::string>(), "link.libraries", cmake_id_to_target, cmake_name_to_target);
                                }
                                else if (lib.is_object())
                                {
                                    if (lib.contains("name") && lib.at("name").is_string())
                                    {
                                        push_edge(g, bt.id, cfg_name, lib.at("name").get<std::string>(), "link.libraries.name", cmake_id_to_target, cmake_name_to_target);
                                    }
                                    else if (lib.contains("path") && lib.at("path").is_string())
                                    {
                                        const std::string p = lib.at("path").get<std::string>();
                                        push_edge(g, bt.id, cfg_name, p, "link.libraries.path", cmake_id_to_target, cmake_name_to_target);
                                    }
                                }
                            }
                        }

                        if (link.contains("commandFragments") && link.at("commandFragments").is_array())
                        {
                            for (const auto &frag : link.at("commandFragments"))
                            {
                                if (!frag.is_object())
                                    continue;
                                if (!frag.contains("fragment") || !frag.at("fragment").is_string())
                                    continue;

                                const std::string fragment = frag.at("fragment").get<std::string>();

                                for (const auto &tok : split_ws(fragment))
                                {
                                    push_edge(g, bt.id, cfg_name, tok, "link.commandFragments.fragment", cmake_id_to_target, cmake_name_to_target);
                                }
                            }
                        }
                    }
                    catch (const std::exception &e)
                    {
                        throw std::runtime_error("Failed while processing target file '" + tgt_path.string() +
                                                 "' at stage 'target processing': " + e.what());
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("Failed while processing codemodel file '" + codemodel_path.string() +
                                     "' at stage 'codemodel processing': " + e.what());
        }

        return g;
    }
}
