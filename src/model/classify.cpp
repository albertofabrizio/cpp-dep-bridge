#include "depbridge/model/classify.hpp"
#include "depbridge/model/evidence.hpp"

#include <unordered_set>

namespace depbridge::model
{
    void classify_project_local_components(ProjectGraph &g, const ClassifyOptions &)
    {
        std::unordered_set<std::string> target_names;
        target_names.reserve(g.targets.size());
        for (const auto &[_, t] : g.targets)
        {
            if (!t.name.empty())
                target_names.insert(t.name);
        }

        for (auto &[_, c] : g.components)
        {
            if (c.origin != ComponentOrigin::unknown)
                continue;
            if (target_names.find(c.name) != target_names.end())
            {
                c.origin = ComponentOrigin::project_local;
                c.sources.push_back(SourceRef{"project-target", c.name, std::nullopt});
            }
        }
    }
}

namespace depbridge::model
{
    namespace
    {
#if defined(_WIN32)
        const std::unordered_set<std::string> system_libs = {
            "kernel32", "user32", "gdi32", "advapi32", "shell32", "ole32", "oleaut32",
            "uuid", "winspool", "comdlg32", "ws2_32", "bcrypt", "crypt32"};
#elif defined(__linux__)
        const std::unordered_set<std::string> system_libs = {
            "c", "m", "dl", "pthread", "rt", "gcc_s", "stdc++"};
#elif defined(__APPLE__)
        const std::unordered_set<std::string> system_libs = {
            "System", "objc", "c++"};
#else
        const std::unordered_set<std::string> system_libs = {};
#endif

        bool has_explicit_system_evidence(const Component &c)
        {
            for (const auto &s : c.sources)
            {
                const auto kind = evidence_kind_from_source(s);
                if (kind == EvidenceKind::cmake_link_token || kind == EvidenceKind::cmake_link_path ||
                    kind == EvidenceKind::system_library_name)
                {
                    return true;
                }
            }
            return false;
        }
    }

    void classify_system_components(ProjectGraph &g)
    {
        for (auto &[_, c] : g.components)
        {
            if (c.origin != ComponentOrigin::unknown)
                continue;

            if (!has_explicit_system_evidence(c))
                continue;

            if (system_libs.find(c.name) != system_libs.end())
            {
                c.origin = ComponentOrigin::system;
                c.sources.push_back(SourceRef{"system-lib", c.name, std::nullopt});
            }
        }
    }
}

namespace depbridge::model
{
    namespace
    {
        bool has_explicit_third_party_evidence(const Component &c)
        {
            for (const auto &s : c.sources)
            {
                const auto kind = evidence_kind_from_source(s);
                if (kind == EvidenceKind::cmake_imported_target ||
                    kind == EvidenceKind::package_manager ||
                    kind == EvidenceKind::package_manager_path)
                {
                    return true;
                }
            }
            return false;
        }
    }

    void classify_third_party_components(ProjectGraph &g)
    {
        for (auto &[_, c] : g.components)
        {
            if (c.origin != ComponentOrigin::unknown)
                continue;

            if (has_explicit_third_party_evidence(c))
            {
                c.origin = ComponentOrigin::third_party;
            }
        }
    }
}
