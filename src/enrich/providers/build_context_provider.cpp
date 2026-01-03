#include "depbridge/enrich/providers/build_context_provider.hpp"

#include <algorithm>
#include <set>

namespace depbridge::enrich::providers
{
    namespace
    {
        template <class T>
        void sort_unique(std::vector<T> &v)
        {
            std::sort(v.begin(), v.end());
            v.erase(std::unique(v.begin(), v.end()), v.end());
        }

        void add_field_sorted(
            std::vector<MetaField> &fields,
            MetaField f)
        {
            fields.push_back(std::move(f));
        }

        Provenance make_provenance(
            const std::string &provider,
            const std::string &provider_version,
            const std::string &rule_id,
            std::vector<Evidence> evidence,
            double confidence = 1.0)
        {
            std::sort(evidence.begin(), evidence.end(),
                      [](const Evidence &a, const Evidence &b)
                      {
                          if (a.kind != b.kind)
                              return a.kind < b.kind;
                          if (a.ref != b.ref)
                              return a.ref < b.ref;
                          return a.note < b.note;
                      });

            Provenance p;
            p.provider = provider;
            p.provider_version = provider_version;
            p.rule_id = rule_id;
            p.confidence = confidence;
            p.evidence = std::move(evidence);
            return p;
        }
    }

    void BuildContextProvider::enrich(
        const depbridge::model::ProjectGraph &graph,
        const EnrichmentConfig &cfg,
        EnrichmentOverlay &out) const
    {
        if (!cfg.enabled)
            return;
        std::map<std::string, std::set<std::string>> component_to_targets;

        for (const auto &e : graph.edges)
        {
            if (!e.to_component)
                continue;

            component_to_targets[e.to_component->value].insert(e.from.value);
        }

        for (const auto &[comp_id, target_ids] : component_to_targets)
        {
            std::vector<std::string> configs;
            std::vector<std::string> toolchains;
            std::vector<std::string> platforms;

            std::vector<Evidence> evidence;
            evidence.reserve(target_ids.size());

            for (const auto &tid : target_ids)
            {
                evidence.push_back(Evidence{
                    "target",
                    tid,
                    "consumes component"});

                auto it = graph.targets.find(tid);
                if (it == graph.targets.end())
                    continue;

                const auto &t = it->second;
                if (t.configuration)
                    configs.push_back(*t.configuration);
                if (t.toolchain)
                    toolchains.push_back(*t.toolchain);
                if (t.platform)
                    platforms.push_back(*t.platform);
            }

            sort_unique(configs);
            sort_unique(toolchains);
            sort_unique(platforms);

            auto &fields = out.component_fields[comp_id];

            const auto prov = make_provenance(name(), version(),
                                              "AGG_FROM_CONSUMING_TARGETS",
                                              evidence);

            if (!configs.empty())
            {
                add_field_sorted(fields, MetaField{
                                             "depbridge:enrich.build.configurations",
                                             configs,
                                             prov});
            }

            if (!toolchains.empty())
            {
                add_field_sorted(fields, MetaField{
                                             "depbridge:enrich.build.toolchains",
                                             toolchains,
                                             prov});
            }

            if (!platforms.empty())
            {
                add_field_sorted(fields, MetaField{
                                             "depbridge:enrich.build.platforms",
                                             platforms,
                                             prov});
            }

            add_field_sorted(fields, MetaField{
                                         "depbridge:enrich.build.used_by_targets_count",
                                         static_cast<int64_t>(target_ids.size()),
                                         prov});

            std::sort(fields.begin(), fields.end(),
                      [](const MetaField &a, const MetaField &b)
                      {
                          return a.key < b.key;
                      });
        }
    }
}
