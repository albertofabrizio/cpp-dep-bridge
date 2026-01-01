#include "depbridge/sbom/cyclonedx_writer.hpp"

#include <iomanip>
#include <sstream>
#include <algorithm>

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

    } // namespace

    void write_cyclonedx_json(std::ostream &os, const ProjectGraph &g)
    {
        std::vector<const Component *> comps;
        comps.reserve(g.components.size());
        for (const auto &[_, c] : g.components)
        {
            comps.push_back(&c);
        }

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

        // Metadata
        indent(os, 2);
        os << "\"metadata\": {\n";
        indent(os, 4);
        os << "\"tools\": [{\n";
        indent(os, 6);
        os << "\"vendor\": \"cpp-dep-bridge\",\n";
        indent(os, 6);
        os << "\"name\": \"cpp-dep-bridge\",\n";
        indent(os, 6);
        os << "\"version\": \"0.1.0-dev\"\n";
        indent(os, 4);
        os << "}]\n";
        indent(os, 2);
        os << "},\n";

        // Components
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
        os << "]\n";
        os << "}\n";
    }

}
