#include "depbridge/model/ids.hpp"

#include <cstdint>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace depbridge::model
{

    namespace
    {

        // Not cryptographic; sufficient for stable identifiers in this context.
        constexpr std::uint64_t fnv1a_offset = 14695981039346656037ull;
        constexpr std::uint64_t fnv1a_prime = 1099511628211ull;

        std::uint64_t fnv1a64(std::string_view s)
        {
            std::uint64_t h = fnv1a_offset;
            for (unsigned char c : s)
            {
                h ^= static_cast<std::uint64_t>(c);
                h *= fnv1a_prime;
            }
            return h;
        }

        std::string hex_u64(std::uint64_t v)
        {
            std::ostringstream oss;
            oss << std::hex << std::nouppercase << std::setfill('0') << std::setw(16) << v;
            return oss.str();
        }

        std::string normalize_field(std::string_view in)
        {
            std::string s(in);
            for (char &ch : s)
            {
                if (ch == '\n' || ch == '\r' || ch == '\t')
                    ch = ' ';
            }
            // trim
            auto not_space = [](unsigned char c)
            { return c != ' '; };
            auto b = std::find_if(s.begin(), s.end(), not_space);
            auto e = std::find_if(s.rbegin(), s.rend(), not_space).base();
            if (b >= e)
                return {};
            s = std::string(b, e);

            std::string out;
            out.reserve(s.size());
            bool prev_space = false;
            for (char ch : s)
            {
                if (ch == ' ')
                {
                    if (!prev_space)
                        out.push_back(' ');
                    prev_space = true;
                }
                else
                {
                    out.push_back(ch);
                    prev_space = false;
                }
            }
            return out;
        }

        std::string component_type_to_string(ComponentType t)
        {
            switch (t)
            {
            case ComponentType::library:
                return "library";
            case ComponentType::executable:
                return "executable";
            case ComponentType::header_only:
                return "header-only";
            case ComponentType::framework:
                return "framework";
            case ComponentType::tool:
                return "tool";
            case ComponentType::system:
                return "system";
            case ComponentType::unknown:
                return "unknown";
            }
            return "unknown";
        }

    }

    std::string canonical_component_key(
        ComponentType type,
        std::string_view namespace_,
        std::string_view name,
        std::string_view version,
        std::string_view purl)
    {
        const std::string p = normalize_field(purl);
        if (!p.empty())
        {
            return "purl=" + p + ";type=" + component_type_to_string(type);
        }

        const std::string ns = normalize_field(namespace_);
        const std::string nm = normalize_field(name);
        const std::string ver = normalize_field(version);

        return "type=" + component_type_to_string(type) +
               ";ns=" + ns +
               ";name=" + nm +
               ";ver=" + ver;
    }

    std::string canonical_target_key(
        std::string_view buildsystem,
        std::string_view project,
        std::string_view target_name,
        std::string_view configuration)
    {
        const std::string bs = normalize_field(buildsystem);
        const std::string pr = normalize_field(project);
        const std::string tn = normalize_field(target_name);
        const std::string cfg = normalize_field(configuration);

        return "bs=" + bs +
               ";project=" + pr +
               ";target=" + tn +
               ";cfg=" + cfg;
    }

    ComponentId make_component_id(
        ComponentType type,
        std::string_view namespace_,
        std::string_view name,
        std::string_view version,
        std::string_view purl)
    {
        const std::string key = canonical_component_key(type, namespace_, name, version, purl);
        const std::uint64_t h = fnv1a64(key);
        return ComponentId{"cmp_" + hex_u64(h)};
    }

    TargetId make_target_id(
        std::string_view buildsystem,
        std::string_view project,
        std::string_view target_name,
        std::string_view configuration)
    {
        const std::string key = canonical_target_key(buildsystem, project, target_name, configuration);
        const std::uint64_t h = fnv1a64(key);
        return TargetId{"tgt_" + hex_u64(h)};
    }

    ComponentId component_id_of(const Component &c)
    {
        return make_component_id(
            c.type,
            c.namespace_.value_or(std::string{}),
            c.name,
            c.version.value_or(std::string{}),
            c.purl.value_or(std::string{}));
    }

}
