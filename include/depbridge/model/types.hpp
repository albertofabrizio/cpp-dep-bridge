#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>

namespace depbridge::model
{

    // ---------- Enumerations ----------

    enum class ComponentType
    {
        library,
        executable,
        header_only,
        framework,
        tool,
        system,
        unknown
    };

    enum class Scope
    {
        runtime,
        build,
        test,
        dev,
        optional_
    };

    enum class Linkage
    {
        static_,
        shared_,
        interface_,
        unknown
    };

    enum class TargetKind
    {
        exe,
        static_lib,
        shared_lib,
        interface_lib,
        object_lib,
        unknown
    };

    enum class ComponentOrigin
    {
        unknown,
        system,
        third_party,
        project_local
    };

    // ---------- Provenance ----------

    struct SourceRef
    {
        std::string system;      // cmake-file-api, conan, vcpkg, manual
        std::string ref;         // file path, object id, lockfile entry
        std::optional<int> line; // optional line number
    };

    // ---------- Variant Evidence ----------
    struct VariantEvidence
    {
        std::string kind;  // e.g. "debug", "release", "static", "shared"
        std::string value; // e.g. "path:/debug/", "msvc-suffix-d"
        SourceRef source;  // where we observed this
    };

    // ---------- Identity ----------

    struct ComponentId
    {
        std::string value; // canonical, stable identifier
    };

    struct TargetId
    {
        std::string value; // canonical, stable identifier
    };

    // ---------- Metadata ----------

    struct Checksum
    {
        std::string algorithm; // sha256, sha1, etc.
        std::string value;
    };

    struct LicenseInfo
    {
        std::optional<std::string> spdx_id; // MIT, Apache-2.0, etc.
        std::optional<std::string> expression;
        std::vector<SourceRef> sources;
    };

    // ---------- Artifacts ----------

    struct Artifact
    {
        std::string path; // normalized path
        std::optional<Checksum> checksum;
        std::vector<SourceRef> sources;
    };

    // ---------- Components ----------

    struct Component
    {
        ComponentId id;

        std::string name;
        std::optional<std::string> namespace_;
        std::optional<std::string> version;
        ComponentType type{ComponentType::unknown};
        ComponentOrigin origin{ComponentOrigin::unknown};

        // SBOM identities
        std::optional<std::string> purl;
        std::optional<std::string> cpe;

        // Metadata
        std::optional<std::string> description;
        std::optional<std::string> homepage;
        std::optional<std::string> supplier;
        LicenseInfo license;

        std::vector<Checksum> checksums;
        std::optional<Linkage> linkage;

        std::map<std::string, std::string> properties; // tool-specific metadata
        std::vector<SourceRef> sources;
    };

    // ---------- Helper ----------

    inline const char *to_string(ComponentOrigin o)
    {
        switch (o)
        {
        case ComponentOrigin::unknown:
            return "unknown";
        case ComponentOrigin::system:
            return "system";
        case ComponentOrigin::third_party:
            return "third-party";
        case ComponentOrigin::project_local:
            return "project-local";
        }
        return "unknown";
    }

    // ---------- Build Targets ----------

    struct BuildTarget
    {
        TargetId id;
        std::string name;
        TargetKind kind{TargetKind::unknown};

        std::optional<std::string> configuration;
        std::optional<std::string> toolchain;
        std::optional<std::string> platform;

        std::vector<std::string> include_dirs;
        std::vector<std::string> compile_definitions;
        std::vector<std::string> compile_options;

        std::vector<Artifact> outputs;
        std::vector<SourceRef> sources;
    };

    // ---------- Dependency Edges ----------

    struct DependencyEdge
    {
        TargetId from;

        std::optional<TargetId> to_target;
        std::optional<ComponentId> to_component;

        Scope scope{Scope::runtime};
        Linkage linkage{Linkage::unknown};

        std::optional<std::string> raw; // raw evidence (e.g. linker token)
        std::vector<SourceRef> sources;
    };

    // ---------- Resolution Context ----------

    struct ResolutionContext
    {
        std::string run_id;
        std::string root_directory;
        std::string build_directory;

        std::optional<std::string> generator;
        std::optional<std::string> cmake_version;

        std::map<std::string, std::string> environment;
        std::vector<SourceRef> sources;
    };

    // ---------- Project Graph ----------

    struct ProjectGraph
    {
        ResolutionContext context;

        std::map<std::string, BuildTarget> targets;  // key = TargetId.value
        std::map<std::string, Component> components; // key = ComponentId.value
        std::vector<DependencyEdge> edges;

        std::vector<std::string> warnings;
    };

} // namespace depbridge::model
