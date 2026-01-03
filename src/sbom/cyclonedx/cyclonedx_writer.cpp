#include "depbridge/sbom/cyclonedx_writer.hpp"

#include <algorithm>
#include <type_traits>

namespace depbridge::sbom
{
    using namespace depbridge::model;
    using namespace depbridge::enrich;

    namespace
    {
        std::string json_escape(const std::string &s)
        {
            std::string out;
            out.reserve(s.size() + 8);
            for (char c : s)
            {
                switch (c)
                {
                case '"':
                    out += "\\\"";
                    break;
                case '\\':
                    out += "\\\\";
                    break;
                case '\n':
                    out += "\\n";
                    break;
                case '\r':
                    out += "\\r";
                    break;
                case '\t':
                    out += "\\t";
                    break;
                default:
                    out += c;
                    break;
                }
            }
            return out;
        }

        void indent(std::ostream &os, int n)
        {
            for (int i = 0; i < n; ++i)
                os.put(' ');
        }

        std::string component_type_to_cdx(ComponentType t)
        {
            switch (t)
            {
            case ComponentType::library:
                return "library";
            case ComponentType::executable:
                return "application";
            case ComponentType::framework:
                return "framework";
            case ComponentType::tool:
                return "tool";
            case ComponentType::system:
                return "operating-system";
            case ComponentType::header_only:
                return "library";
            case ComponentType::unknown:
                return "library";
            }
            return "library";
        }

        std::string meta_value_to_string(const MetaValue &v)
        {
            return std::visit([](auto &&val) -> std::string
                              {
                using T = std::decay_t<decltype(val)>;

                if constexpr (std::is_same_v<T, std::string>)
                    return val;
                else if constexpr (std::is_same_v<T, bool>)
                    return val ? "true" : "false";
                else if constexpr (std::is_same_v<T, int64_t>)
                    return std::to_string(val);
                else if constexpr (std::is_same_v<T, double>)
                    return std::to_string(val);
                else if constexpr (std::is_same_v<T, std::vector<std::string>>)
                {
                    std::string out;
                    for (std::size_t i = 0; i < val.size(); ++i)
                    {
                        if (i) out += ",";
                        out += val[i];
                    }
                    return out;
                } }, v);
        }
    }

    void write_cyclonedx_json(std::ostream &os, const ProjectGraph &g)
    {
        EnrichmentOverlay empty;
        write_cyclonedx_json(os, g, empty);
    }

    void write_cyclonedx_json(
        std::ostream &os,
        const ProjectGraph &g,
        const EnrichmentOverlay &enrich)
    {
        std::vector<const Component *> comps;
        for (const auto &[_, c] : g.components)
            comps.push_back(&c);

        std::sort(comps.begin(), comps.end(),
                  [](const Component *a, const Component *b)
                  {
                      return a->id.value < b->id.value;
                  });

        os << "{\n";
        indent(os, 2);
        os << "\"bomFormat\": \"CycloneDX\",\n";
        indent(os, 2);
        os << "\"specVersion\": \"1.5\",\n";
        indent(os, 2);
        os << "\"version\": 1,\n";

        indent(os, 2);
        os << "\"metadata\": {\n";
        indent(os, 4);
        os << "\"tools\": [{\n";
        indent(os, 6);
        os << "\"vendor\": \"cpp-dep-bridge\",\n";
        indent(os, 6);
        os << "\"name\": \"cpp-dep-bridge\",\n";
        indent(os, 6);
        os << "\"version\": \"0.4.0-alpha\"\n";
        indent(os, 4);
        os << "}]\n";
        indent(os, 2);
        os << "},\n";

        indent(os, 2);
        os << "\"components\": [\n";

        for (std::size_t i = 0; i < comps.size(); ++i)
        {
            const Component &c = *comps[i];

            indent(os, 4);
            os << "{\n";
            indent(os, 6);
            os << "\"type\": \"" << component_type_to_cdx(c.type) << "\",\n";
            indent(os, 6);
            os << "\"bom-ref\": \"" << json_escape(c.id.value) << "\",\n";
            indent(os, 6);
            os << "\"name\": \"" << json_escape(c.name) << "\"";

            if (c.version)
            {
                os << ",\n";
                indent(os, 6);
                os << "\"version\": \"" << json_escape(*c.version) << "\"";
            }

            os << ",\n";
            indent(os, 6);
            os << "\"properties\": [\n";
            indent(os, 8);
            os << "{ \"name\": \"depbridge:origin\", \"value\": \""
               << json_escape(to_string(c.origin)) << "\" }";

            auto it = enrich.component_fields.find(c.id.value);
            if (it != enrich.component_fields.end())
            {
                for (const auto &f : it->second)
                {
                    os << ",\n";
                    indent(os, 8);
                    os << "{ \"name\": \"" << json_escape(f.key)
                       << "\", \"value\": \""
                       << json_escape(meta_value_to_string(f.value))
                       << "\" }";
                }
                os << "\n";
            }
            else
            {
                os << "\n";
            }

            indent(os, 6);
            os << "]\n";
            indent(os, 4);
            os << "}";

            if (i + 1 < comps.size())
                os << ",";
            os << "\n";
        }

        indent(os, 2);
        os << "]\n";
        os << "}\n";
    }
}
