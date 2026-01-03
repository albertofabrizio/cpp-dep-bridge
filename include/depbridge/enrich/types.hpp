#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace depbridge::enrich
{
    struct Evidence
    {
        std::string kind; // "path", "manifest", "flag"
        std::string ref;  // normalized reference
        std::string note; // short explanation
    };

    struct Provenance
    {
        std::string provider;
        std::string provider_version;
        std::string rule_id;
        double confidence{1.0};
        std::vector<Evidence> evidence;
    };

    using MetaValue = std::variant<
        std::string,
        bool,
        int64_t,
        double,
        std::vector<std::string>>;

    struct MetaField
    {
        std::string key; // depbridge:enrich.*
        MetaValue value;
        Provenance provenance;
    };

    struct EnrichmentOverlay
    {
        // Fingerprint of the ProjectGraph this overlay applies to
        std::string model_fingerprint;

        // key = ComponentId.value
        std::map<std::string, std::vector<MetaField>> component_fields;

        // Hash of (model_fingerprint + config + providers + fields)
        // Empty in Phase-1 Step-1
        std::string overlay_hash;
    };
}
