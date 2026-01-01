#include "depbridge/model/variant_normalize.hpp"
#include "depbridge/model/ids.hpp"
#include "depbridge/model/normalize.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>

namespace depbridge::model
{
    namespace
    {
        std::string to_lower_ascii(std::string s)
        {
            for (char &ch : s)
            {
                if (ch >= 'A' && ch <= 'Z')
                    ch = static_cast<char>(ch - 'A' + 'a');
            }
            return s;
        }

        std::string normalize_slashes(std::string s)
        {
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

        bool contains_ci(std::string_view hay, std::string_view needle)
        {
            std::string h = to_lower_ascii(normalize_slashes(std::string(hay)));
            std::string n = to_lower_ascii(normalize_slashes(std::string(needle)));
            return h.find(n) != std::string::npos;
        }

        bool infer_debug_from_sources(const Component &c)
        {
            for (const auto &s : c.sources)
            {
                if (contains_ci(s.ref, "/debug/") || contains_ci(s.ref, "\\debug\\"))
                    return true;
            }
            return false;
        }

        bool looks_like_msvc_lib_artifact(const Component &c)
        {
            for (const auto &s : c.sources)
            {
                if (contains_ci(s.ref, ".lib"))
                    return true;
            }
            return false;
        }

        void prop_append(std::map<std::string, std::string> &props,
                         const std::string &key,
                         const std::string &value)
        {
            auto it = props.find(key);
            if (it == props.end() || it->second.empty())
            {
                props[key] = value;
                return;
            }

            const std::string &cur = it->second;
            std::string needle = ";" + value + ";";
            std::string hay = ";" + cur + ";";
            if (hay.find(needle) == std::string::npos)
            {
                it->second += ";";
                it->second += value;
            }
        }

        void canonicalize_and_record(Component &c, const VariantNormalizeOptions &opt)
        {
            if (!opt.enable)
                return;

            bool debug = false;

            if (opt.infer_debug_from_paths && infer_debug_from_sources(c))
            {
                debug = true;
                if (opt.record_evidence_in_properties)
                {
                    prop_append(c.properties, "depbridge:variant", "debug");
                    prop_append(c.properties, "depbridge:variant.evidence", "path:/debug/");
                }
            }

            if (opt.msvc_strip_debug_suffix_d && debug)
            {
                if (looks_like_msvc_lib_artifact(c))
                {
                    if (c.name.size() > 1 && c.name.back() == 'd')
                    {
                        if (opt.record_evidence_in_properties)
                        {
                            prop_append(c.properties, "depbridge:variant", "debug");
                            prop_append(c.properties, "depbridge:variant.evidence", "msvc-suffix-d");
                            prop_append(c.properties, "depbridge:variant.original-name", c.name);
                        }

                        c.name.pop_back();
                    }
                }
            }
        }

        void merge_component_variant_props(Component &dst, const Component &incoming)
        {
            merge_component(dst, incoming);

            auto merge_prop = [&](const char *k)
            {
                auto it = incoming.properties.find(k);
                if (it == incoming.properties.end())
                    return;
                prop_append(dst.properties, k, it->second);
            };

            merge_prop("depbridge:variant");
            merge_prop("depbridge:variant.evidence");
            merge_prop("depbridge:variant.original-name");
        }

    }

    void normalize_build_variants(ProjectGraph &g, const VariantNormalizeOptions &opt)
    {
        if (!opt.enable)
            return;

        std::unordered_map<std::string, std::string> remap_old_to_new;
        std::map<std::string, Component> rebuilt;

        for (auto &[old_key, comp] : g.components)
        {
            Component canon = comp;

            canonicalize_and_record(canon, opt);

            const ComponentId new_id = component_id_of(canon);
            canon.id = new_id;

            remap_old_to_new[old_key] = new_id.value;
            remap_old_to_new[comp.id.value] = new_id.value;

            auto it = rebuilt.find(new_id.value);
            if (it == rebuilt.end())
            {
                rebuilt.emplace(new_id.value, std::move(canon));
            }
            else
            {
                merge_component_variant_props(it->second, canon);
            }
        }

        g.components = std::move(rebuilt);

        for (auto &e : g.edges)
        {
            if (!e.to_component)
                continue;

            const auto it = remap_old_to_new.find(e.to_component->value);
            if (it != remap_old_to_new.end())
            {
                e.to_component = ComponentId{it->second};
            }
        }
    }

}
