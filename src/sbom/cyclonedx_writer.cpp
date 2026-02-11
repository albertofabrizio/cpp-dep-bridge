#include "depbridge/sbom/cyclonedx_writer.hpp"

#include "depbridge/version.hpp"

#include <algorithm>
#include <map>
#include <set>

namespace depbridge::sbom
{

    using namespace depbridge::model;

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

        std::optional<const Component *> resolve_metadata_subject(const ProjectGraph &g)
        {
            for (const auto &[_, t] : g.targets)
            {
                for (const auto &[__, c] : g.components)
                {
                    if (c.name == t.name)
                    {
                        return &c;
                    }
                }
            }
            return std::nullopt;
        }

    } // namespace

    void write_cyclonedx_json(std::ostream &os, const ProjectGraph &g)
    {
        std::vector<const Component *> comps;
        comps.reserve(g.components.size());
        for (const auto &[_, c] : g.components)
        {
            comps.push_back(&c);
        }

        std::sort(comps.begin(), comps.end(), [](const Component *a, const Component *b)
                  { return a->id.value < b->id.value; });

        std::map<std::string, std::set<std::string>> dependencies;
        std::map<std::string, std::set<std::string>> target_to_components;

        for (const auto &edge : g.edges)
        {
            if (edge.to_component)
            {
                target_to_components[edge.from.value].insert(edge.to_component->value);
            }
        }

        for (const auto &[target_id, dep_components] : target_to_components)
        {
            const auto target_it = g.targets.find(target_id);
            if (target_it == g.targets.end())
                continue;

            for (const auto &[_, component] : g.components)
            {
                if (component.name != target_it->second.name)
                    continue;

                auto &deps = dependencies[component.id.value];
                deps.insert(dep_components.begin(), dep_components.end());
                deps.erase(component.id.value);
            }
        }

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
        os << "\"version\": \"" << json_escape(depbridge::version) << "\"\n";
        indent(os, 4);
        os << "}]";

        if (const auto subject = resolve_metadata_subject(g); subject.has_value())
        {
            os << ",\n";
            indent(os, 4);
            os << "\"component\": {\n";
            indent(os, 6);
            os << "\"type\": \"" << component_type_to_cdx((*subject.value())->type) << "\",\n";
            indent(os, 6);
            os << "\"bom-ref\": \"" << json_escape((*subject.value())->id.value) << "\",\n";
            indent(os, 6);
            os << "\"name\": \"" << json_escape((*subject.value())->name) << "\"\n";
            indent(os, 4);
            os << "}";
        }

        os << "\n";
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

            if (c.purl)
            {
                os << ",\n";
                indent(os, 6);
                os << "\"purl\": \"" << json_escape(*c.purl) << "\"";
            }

            if (c.license.spdx_id)
            {
                os << ",\n";
                indent(os, 6);
                os << "\"licenses\": [{\"license\": {\"id\": \""
                   << json_escape(*c.license.spdx_id) << "\"}}]";
            }

            os << ",\n";
            indent(os, 6);
            os << "\"properties\": [\n";
            indent(os, 8);
            os << "{ \"name\": \"depbridge:origin\", \"value\": \""
               << json_escape(to_string(c.origin)) << "\" }\n";
            indent(os, 6);
            os << "]\n";

            indent(os, 4);
            os << "}";

            if (i + 1 < comps.size())
                os << ",";
            os << "\n";
        }

        indent(os, 2);
        os << "],\n";

        indent(os, 2);
        os << "\"dependencies\": [\n";

        std::vector<std::string> dep_refs;
        dep_refs.reserve(comps.size());
        for (const auto *comp : comps)
            dep_refs.push_back(comp->id.value);

        for (std::size_t i = 0; i < dep_refs.size(); ++i)
        {
            const auto &ref = dep_refs[i];
            indent(os, 4);
            os << "{\n";
            indent(os, 6);
            os << "\"ref\": \"" << json_escape(ref) << "\",\n";
            indent(os, 6);
            os << "\"dependsOn\": [";

            std::vector<std::string> sorted_deps;
            if (const auto it = dependencies.find(ref); it != dependencies.end())
            {
                sorted_deps.assign(it->second.begin(), it->second.end());
            }

            for (std::size_t di = 0; di < sorted_deps.size(); ++di)
            {
                if (di > 0)
                    os << ", ";
                os << "\"" << json_escape(sorted_deps[di]) << "\"";
            }

            os << "]\n";
            indent(os, 4);
            os << "}";
            if (i + 1 < dep_refs.size())
                os << ",";
            os << "\n";
        }

        indent(os, 2);
        os << "]\n";
        os << "}\n";
    }

}
