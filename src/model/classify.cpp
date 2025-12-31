#include "depbridge/model/classify.hpp"

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
            "kernel32",
            "user32",
            "gdi32",
            "advapi32",
            "shell32",
            "ole32",
            "oleaut32",
            "uuid",
            "winspool",
            "comdlg32",
            "ws2_32",
            "bcrypt",
            "crypt32"};

#elif defined(__linux__)

        const std::unordered_set<std::string> system_libs = {
            "c",
            "m",
            "dl",
            "pthread",
            "rt",
            "gcc_s",
            "stdc++"};

#elif defined(__APPLE__)

        const std::unordered_set<std::string> system_libs = {
            "System",
            "objc",
            "c++"};

#else

        const std::unordered_set<std::string> system_libs = {};

#endif

    } // namespace

    void classify_system_components(ProjectGraph &g)
    {
        for (auto &[_, c] : g.components)
        {
            if (c.origin != ComponentOrigin::unknown)
                continue;

            if (system_libs.find(c.name) != system_libs.end())
            {
                c.origin = ComponentOrigin::system;
            }
        }
    }

}
