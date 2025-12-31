#include "depbridge/model/normalize.hpp"
#include "depbridge/model/ids.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace depbridge::model
{
    namespace
    {
        bool is_space(char c) { return c == ' '; }

        std::string trim_and_collapse_spaces(std::string s)
        {
            for (char &ch : s)
            {
                if (ch == '\n' || ch == '\r' || ch == '\t')
                    ch = ' ';
            }

            auto not_space = [](unsigned char c)
            { return c != ' '; };
            auto b = std::find_if(s.begin(), s.end(), not_space);
            auto e = std::find_if(s.rbegin(), s.rend(), not_space).base();
            if (b >= e)
                return {};
            s = std::string(b, e);

            std::string out;
            out.reserve(s.size());
            bool prev = false;
            for (char ch : s)
            {
                if (ch == ' ')
                {
                    if (!prev)
                        out.push_back(' ');
                    prev = true;
                }
                else
                {
                    out.push_back(ch);
                    prev = false;
                }
            }
            return out;
        }

        static bool ends_with(std::string_view s, std::string_view suf)
        {
            return s.size() >= suf.size() && s.substr(s.size() - suf.size()) == suf;
        }

        static bool starts_with(std::string_view s, std::string_view pre)
        {
            return s.size() >= pre.size() && s.substr(0, pre.size()) == pre;
        }

        std::string to_lower_ascii(std::string s)
        {
            for (char &ch : s)
            {
                if (ch >= 'A' && ch <= 'Z')
                    ch = static_cast<char>(ch - 'A' + 'a');
            }
            return s;
        }

        std::string strip_ext(std::string s, const NormalizeOptions &opt)
        {
            if (!opt.strip_library_extensions)
                return s;
            const std::string lower = to_lower_ascii(s);
            if (ends_with(lower, ".a"))
                return s.substr(0, s.size() - 2);
            if (ends_with(lower, ".so"))
                return s.substr(0, s.size() - 3);
            if (ends_with(lower, ".dylib"))
                return s.substr(0, s.size() - 6);
            if (ends_with(lower, ".lib"))
                return s.substr(0, s.size() - 4);
            return s;
        }

        std::string basename_noext(std::string_view path)
        {
            const auto pos = path.find_last_of('/');
            std::string base = (pos == std::string_view::npos) ? std::string(path) : std::string(path.substr(pos + 1));
            return base;
        }

        std::string strip_unix_libprefix(std::string s, const NormalizeOptions &opt)
        {
            if (!opt.strip_unix_lib_prefix)
                return s;
            if (starts_with(s, "lib") && s.size() > 3)
            {
                return s.substr(3);
            }
            return s;
        }

        std::string normalize_lib_name_from_file(std::string_view normalized_path, const NormalizeOptions &opt)
        {
            std::string base = basename_noext(normalized_path);
            base = strip_ext(base, opt);
            base = strip_unix_libprefix(base, opt);
            if (opt.case_fold_windows_libs)
            {
                base = to_lower_ascii(base);
            }
            return base;
        }

        bool looks_like_path(std::string_view s)
        {
            return s.find('/') != std::string_view::npos || s.find('\\') != std::string_view::npos;
        }

        bool looks_like_cmake_target(std::string_view s)
        {
            return s.find("::") != std::string_view::npos;
        }

        bool is_imported_cmake_target(std::string_view raw)
        {
            return looks_like_cmake_target(raw);
        }

        std::string imported_target_namespace(std::string_view raw)
        {
            const auto pos = raw.find("::");
            if (pos == std::string_view::npos)
                return std::string(raw);
            return std::string(raw.substr(0, pos));
        }

    }

    std::string normalize_path(std::string_view path)
    {
        std::string s(path);
        for (char &ch : s)
        {
            if (ch == '\\')
                ch = '/';
        }
        std::string out;
        out.reserve(s.size());
        char prev = 0;
        for (char ch : s)
        {
            if (ch == '/' && prev == '/')
                continue;
            out.push_back(ch);
            prev = ch;
        }
        return out;
    }

    std::string normalize_token(std::string_view token)
    {
        return trim_and_collapse_spaces(std::string(token));
    }

    Component component_from_link_token(std::string_view raw_token, const NormalizeOptions &opt)
    {
        Component c;
        c.type = ComponentType::library;

        const std::string tok = normalize_token(raw_token);
        c.sources.push_back(SourceRef{"link-token", tok, std::nullopt});

        if (tok.empty())
        {
            c.name = "";
            c.type = ComponentType::unknown;
            c.id = make_component_id(c.type, "", c.name, "", "");
            return c;
        }

        if (starts_with(tok, "-l") && tok.size() > 2)
        {
            c.name = tok.substr(2);
            c.id = make_component_id(c.type, "", c.name, "", "");
            return c;
        }

        if (looks_like_path(tok))
        {
            const std::string p = normalize_path(tok);
            c.name = normalize_lib_name_from_file(p, opt);
            c.id = make_component_id(c.type, "", c.name, "", "");
            return c;
        }

        if (is_imported_cmake_target(tok))
        {
            c.type = ComponentType::library;
            c.name = imported_target_namespace(tok);

            c.properties.emplace("cmake.target", tok);
            c.sources.push_back(SourceRef{"cmake", "imported-target", std::nullopt});

            c.id = make_component_id(c.type, "", c.name, "", "");
            return c;
        }

        {
            std::string name = tok;
            name = strip_ext(name, opt);
            if (opt.case_fold_windows_libs)
                name = to_lower_ascii(name);
            name = strip_unix_libprefix(name, opt);
            c.name = name;
            c.id = make_component_id(c.type, "", c.name, "", "");
            return c;
        }
    }

    static void append_sources(std::vector<SourceRef> &dst, const std::vector<SourceRef> &src)
    {
        for (const auto &s : src)
        {
            bool exists = false;
            for (const auto &d : dst)
            {
                if (d.system == s.system && d.ref == s.ref && d.line == s.line)
                {
                    exists = true;
                    break;
                }
            }
            if (!exists)
                dst.push_back(s);
        }
    }

    static void append_checksums(std::vector<Checksum> &dst, const std::vector<Checksum> &src)
    {
        for (const auto &s : src)
        {
            bool exists = false;
            for (const auto &d : dst)
            {
                if (d.algorithm == s.algorithm && d.value == s.value)
                {
                    exists = true;
                    break;
                }
            }
            if (!exists)
                dst.push_back(s);
        }
    }

    void merge_component(Component &dst, const Component &incoming)
    {
        if (dst.name.empty() && !incoming.name.empty())
            dst.name = incoming.name;
        if (!dst.namespace_ && incoming.namespace_)
            dst.namespace_ = incoming.namespace_;
        if (!dst.version && incoming.version)
            dst.version = incoming.version;

        if (!dst.purl && incoming.purl)
            dst.purl = incoming.purl;
        if (!dst.cpe && incoming.cpe)
            dst.cpe = incoming.cpe;

        if (!dst.description && incoming.description)
            dst.description = incoming.description;
        if (!dst.homepage && incoming.homepage)
            dst.homepage = incoming.homepage;
        if (!dst.supplier && incoming.supplier)
            dst.supplier = incoming.supplier;

        if (!dst.license.spdx_id && incoming.license.spdx_id)
            dst.license.spdx_id = incoming.license.spdx_id;
        if (!dst.license.expression && incoming.license.expression)
            dst.license.expression = incoming.license.expression;
        append_sources(dst.license.sources, incoming.license.sources);

        if (dst.type == ComponentType::unknown && incoming.type != ComponentType::unknown)
        {
            dst.type = incoming.type;
        }

        if (!dst.linkage && incoming.linkage)
            dst.linkage = incoming.linkage;

        append_sources(dst.sources, incoming.sources);
        append_checksums(dst.checksums, incoming.checksums);

        for (const auto &[k, v] : incoming.properties)
        {
            if (dst.properties.find(k) == dst.properties.end())
            {
                dst.properties.emplace(k, v);
            }
        }
    }

    void normalize_graph(ProjectGraph &g, const NormalizeOptions &opt)
    {
        for (auto &e : g.edges)
        {
            if (e.to_component)
                continue;
            if (!e.raw.has_value())
                continue;

            Component c = component_from_link_token(*e.raw, opt);

            if (c.name.empty())
                continue;

            const ComponentId cid = component_id_of(c);
            c.id = cid;

            auto it = g.components.find(cid.value);
            if (it == g.components.end())
            {
                g.components.emplace(cid.value, std::move(c));
            }
            else
            {
                merge_component(it->second, c);
            }

            e.to_component = cid;
        }

        std::unordered_map<std::string, std::string> remap;
        std::map<std::string, Component> rebuilt;

        for (auto &[old_key, comp] : g.components)
        {
            const ComponentId new_id = component_id_of(comp);
            const std::string new_key = new_id.value;

            remap[old_key] = new_key;
            remap[comp.id.value] = new_key; // in case old key differs from stored id

            comp.id = new_id;

            auto it = rebuilt.find(new_key);
            if (it == rebuilt.end())
            {
                rebuilt.emplace(new_key, comp);
            }
            else
            {
                merge_component(it->second, comp);
            }
        }

        g.components = std::move(rebuilt);

        for (auto &e : g.edges)
        {
            if (e.to_component)
            {
                const auto it = remap.find(e.to_component->value);
                if (it != remap.end())
                {
                    e.to_component = ComponentId{it->second};
                }
            }
        }
    }

}
