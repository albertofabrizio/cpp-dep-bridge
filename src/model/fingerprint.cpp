#include "depbridge/model/fingerprint.hpp"

#include <algorithm>
#include <sstream>

namespace depbridge::model
{
    namespace
    {
        void hash_line(std::ostringstream &os, const std::string &s)
        {
            os << s << '\n';
        }
    }

    std::string compute_fingerprint(const ProjectGraph &g)
    {
        std::ostringstream os;

        for (const auto &[id, c] : g.components)
        {
            hash_line(os, "C:" + id);
            hash_line(os, "N:" + c.name);
            hash_line(os, "T:" + std::to_string(static_cast<int>(c.type)));
            hash_line(os, "O:" + std::to_string(static_cast<int>(c.origin)));

            if (c.version)
                hash_line(os, "V:" + *c.version);
        }

        for (const auto &[id, t] : g.targets)
        {
            hash_line(os, "TGT:" + id);
            hash_line(os, "K:" + std::to_string(static_cast<int>(t.kind)));

            if (t.configuration)
                hash_line(os, "CFG:" + *t.configuration);
            if (t.toolchain)
                hash_line(os, "TC:" + *t.toolchain);
            if (t.platform)
                hash_line(os, "PLAT:" + *t.platform);
        }

        for (const auto &e : g.edges)
        {
            hash_line(os, "E:" + e.from.value);
            if (e.to_target)
                hash_line(os, "ET:" + e.to_target->value);
            if (e.to_component)
                hash_line(os, "EC:" + e.to_component->value);
            hash_line(os, "S:" + std::to_string(static_cast<int>(e.scope)));
            hash_line(os, "L:" + std::to_string(static_cast<int>(e.linkage)));
        }

        return std::to_string(std::hash<std::string>{}(os.str()));
    }
}
