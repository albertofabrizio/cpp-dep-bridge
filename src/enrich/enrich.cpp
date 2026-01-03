#include "depbridge/enrich/enrich.hpp"
#include "depbridge/model/fingerprint.hpp"

#include "depbridge/enrich/providers/build_context_provider.hpp"

#include <algorithm>
#include <sstream>

namespace depbridge::enrich
{
    namespace
    {
        std::string meta_value_to_string(const MetaValue &v)
        {
            struct Visitor
            {
                std::string operator()(const std::string &s) const { return "s:" + s; }
                std::string operator()(bool b) const { return std::string("b:") + (b ? "1" : "0"); }
                std::string operator()(int64_t i) const { return "i:" + std::to_string(i); }
                std::string operator()(double d) const { return "d:" + std::to_string(d); }
                std::string operator()(const std::vector<std::string> &xs) const
                {
                    std::ostringstream os;
                    os << "a[";
                    for (std::size_t i = 0; i < xs.size(); ++i)
                    {
                        if (i)
                            os << ",";
                        os << xs[i];
                    }
                    os << "]";
                    return os.str();
                }
            };
            return std::visit(Visitor{}, v);
        }

        std::string compute_overlay_hash(const EnrichmentOverlay &o)
        {
            std::ostringstream os;
            os << "fp:" << o.model_fingerprint << "\n";

            for (const auto &[comp_id, fields] : o.component_fields)
            {
                os << "comp:" << comp_id << "\n";
                for (const auto &f : fields)
                {
                    os << "k:" << f.key << "\n";
                    os << "v:" << meta_value_to_string(f.value) << "\n";

                    os << "prov:" << f.provenance.provider
                       << "@" << f.provenance.provider_version
                       << ":" << f.provenance.rule_id
                       << ":" << f.provenance.confidence << "\n";

                    for (const auto &ev : f.provenance.evidence)
                    {
                        os << "ev:" << ev.kind << "|" << ev.ref << "|" << ev.note << "\n";
                    }
                }
            }

            return std::to_string(std::hash<std::string>{}(os.str()));
        }
    }

    EnrichmentOverlay enrich(
        const depbridge::model::ProjectGraph &graph,
        const EnrichmentConfig &cfg)
    {
        EnrichmentOverlay out;
        out.model_fingerprint = depbridge::model::compute_fingerprint(graph);

        if (!cfg.enabled)
            return out;

        if (cfg.enable_build_context)
        {
            providers::BuildContextProvider p;
            p.enrich(graph, cfg, out);
        }

        out.overlay_hash = compute_overlay_hash(out);
        return out;
    }
}
