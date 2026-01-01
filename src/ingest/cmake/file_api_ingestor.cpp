#include "depbridge/ingest/cmake/file_api_ingestor.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>
#include <optional>

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

    static std::string to_lower_ascii(std::string s)
    {
        for (char &ch : s)
        {
            if (ch >= 'A' && ch <= 'Z')
                ch = static_cast<char>(ch - 'A' + 'a');
        }
        return s;
    }

    static std::string normalize_slashes(std::string s)
    {
        for (char &ch : s)
        {
            if (ch == '\\')
                ch = '/';
        }
        return s;
    }

    static bool ends_with(std::string_view s, std::string_view suf)
    {
        return s.size() >= suf.size() && s.substr(s.size() - suf.size()) == suf;
    }

    static std::string basename_noext(std::string_view p)
    {
        const auto pos = p.find_last_of("/\\");
        std::string base = (pos == std::string_view::npos) ? std::string(p) : std::string(p.substr(pos + 1));
        auto dot = base.find_last_of('.');
        if (dot != std::string::npos)
            base = base.substr(0, dot);
        return base;
    }

    struct VcpkgHit
    {
        std::string triplet;
        std::string port; // best-effort from lib name
        bool is_debug = false;
        std::string normalized_path;
    };

    static std::optional<VcpkgHit> detect_vcpkg_from_lib_path(std::string_view raw_path)
    {
        std::string p = normalize_slashes(std::string(raw_path));
        std::string pl = to_lower_ascii(p);

        const std::string needle = "/vcpkg/installed/";
        auto pos = pl.find(needle);
        if (pos == std::string::npos)
            return std::nullopt;

        auto triplet_begin = pos + needle.size();
        auto triplet_end = pl.find('/', triplet_begin);
        if (triplet_end == std::string::npos || triplet_end == triplet_begin)
            return std::nullopt;

        const std::string triplet = p.substr(triplet_begin, triplet_end - triplet_begin);

        bool is_debug = (pl.find("/debug/lib/") != std::string::npos);

        if (!ends_with(pl, ".lib"))
            return std::nullopt;

        std::string libname = basename_noext(p);

        if (is_debug && libname.size() > 1 && (libname.back() == 'd' || libname.back() == 'D'))
        {
            libname.pop_back();
        }

        std::string port = to_lower_ascii(libname);

        VcpkgHit hit;
        hit.triplet = triplet;
        hit.port = port;
        hit.is_debug = is_debug;
        hit.normalized_path = p;
        return hit;
    }

    static bool is_noise_token(const std::string &s)
    {
        if (s.empty())
            return true;

        if (s.find("$<") != std::string::npos)
            return true;

        if (!s.empty() && (s[0] == '/' || s[0] == '-'))
            return true;

        return false;
    }

    static std::vector<std::string> split_ws(const std::string &s)
    {
        std::vector<std::string> out;
        std::string cur;
        for (char ch : s)
        {
            if (std::isspace(static_cast<unsigned char>(ch)))
            {
                if (!cur.empty())
                {
                    out.push_back(cur);
                    cur.clear();
                }
            }
            else
            {
                cur.push_back(ch);
            }
        }
        if (!cur.empty())
            out.push_back(cur);
        return out;
    }

    static void push_edge(ProjectGraph &g,
                          const TargetId &from,
                          const std::string &raw,
                          const std::string &ref,
                          const std::vector<SourceRef> &extra_sources = {})
    {
        if (is_noise_token(raw))
            return;

        DependencyEdge e;
        e.from = from;
        e.raw = raw;

        e.sources.push_back(SourceRef{"cmake", ref, std::nullopt});
        for (const auto &s : extra_sources)
            e.sources.push_back(s);

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

                const std::string cmake_tid = tgt.contains("id") && tgt.at("id").is_string()
                                                  ? tgt.at("id").get<std::string>()
                                                  : std::string{};

                bt.sources.push_back(SourceRef{
                    "cmake",
                    "target/" + bt.name,
                    std::nullopt});

                if (!cmake_tid.empty())
                {
                    bt.sources.push_back(SourceRef{
                        "cmake-target-id",
                        cmake_tid,
                        std::nullopt});
                }

                if (!cmake_tid.empty())
                    cmake_id_to_target.emplace(cmake_tid, bt.id);
                cmake_name_to_target.emplace(bt.name, bt.id);

                const bool is_generator = tgt.contains("isGeneratorProvided") &&
                                          tgt.at("isGeneratorProvided").is_boolean() &&
                                          tgt.at("isGeneratorProvided").get<bool>();
                const bool has_artifacts = tgt.contains("artifacts");

                if (!is_generator && !has_artifacts)
                {
                    bt.sources.push_back(SourceRef{
                        "cmake-imported",
                        bt.name,
                        std::nullopt});
                }

                g.targets.emplace(bt.id.value, bt);

                if (!tgt.contains("link"))
                    continue;

                const auto &link = tgt.at("link");

                if (link.contains("libraries") && link.at("libraries").is_array())
                {
                    for (const auto &lib : link.at("libraries"))
                    {
                        if (lib.is_string())
                        {
                            push_edge(g, bt.id, lib.get<std::string>(), "link.libraries");
                        }
                        else if (lib.is_object())
                        {
                            if (lib.contains("name") && lib.at("name").is_string())
                            {
                                push_edge(g, bt.id, lib.at("name").get<std::string>(), "link.libraries.name");
                            }
                            else if (lib.contains("path") && lib.at("path").is_string())
                            {
                                const std::string p = lib.at("path").get<std::string>();

                                std::vector<SourceRef> extra;
                                if (auto hit = detect_vcpkg_from_lib_path(p))
                                {
                                    extra.push_back(SourceRef{"vcpkg", hit->port + ":" + hit->triplet, std::nullopt});
                                    extra.push_back(SourceRef{"vcpkg-path", hit->normalized_path, std::nullopt});
                                }

                                push_edge(g, bt.id, p, "link.libraries.path", extra);
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
                        const std::string role = (frag.contains("role") && frag.at("role").is_string())
                                                     ? frag.at("role").get<std::string>()
                                                     : std::string{};

                        for (const auto &tok : split_ws(fragment))
                        {
                            std::vector<SourceRef> extra;
                            if (role == "libraries")
                            {
                                if (auto hit = detect_vcpkg_from_lib_path(tok))
                                {
                                    extra.push_back(SourceRef{"vcpkg", hit->port + ":" + hit->triplet, std::nullopt});
                                    extra.push_back(SourceRef{"vcpkg-path", hit->normalized_path, std::nullopt});
                                }
                            }

                            push_edge(g, bt.id, tok, "link.commandFragments.fragment", extra);
                        }
                    }
                }
            }
        }

        return g;
    }

}
