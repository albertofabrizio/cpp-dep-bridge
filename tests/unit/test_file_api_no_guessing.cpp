#include "depbridge/ingest/cmake/file_api_ingestor.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

static void write_file(const std::filesystem::path &p, const std::string &content)
{
    std::filesystem::create_directories(p.parent_path());
    std::ofstream out(p);
    out << content;
}

static void test_ingest_does_not_emit_guess_sources()
{
    namespace fs = std::filesystem;
    const fs::path base = fs::temp_directory_path() / "depbridge_file_api_no_guessing";
    fs::remove_all(base);

    const fs::path reply = base / ".cmake" / "api" / "v1" / "reply";

    write_file(reply / "index-0001.json", R"({
      "objects": [
        {"kind":"codemodel","version":{"major":2},"jsonFile":"codemodel-v2.json"}
      ]
    })");

    write_file(reply / "codemodel-v2.json", R"({
      "configurations": [
        {
          "name": "Debug",
          "targets": [
            {"jsonFile":"target-app.json"}
          ]
        }
      ]
    })");

    write_file(reply / "target-app.json", R"({
      "name": "app",
      "id": "app::id",
      "artifacts": [],
      "link": {
        "libraries": [
          {"path":"C:/vcpkg/installed/x64-windows/lib/fmtd.lib"}
        ],
        "commandFragments": [
          {"fragment":"-DNDEBUG -O2 -g","role":"flags"},
          {"fragment":"fmt::fmt","role":"libraries"}
        ]
      }
    })");

    auto g = depbridge::ingest::cmake::ingest_file_api(base, {});
    assert(!g.edges.empty());
    bool saw_expected_library_fragment = false;
    for (const auto &edge : g.edges)
    {
        if (edge.raw.has_value() && *edge.raw == "fmt::fmt")
            saw_expected_library_fragment = true;

        if (edge.raw.has_value())
        {
            assert(*edge.raw != "-DNDEBUG");
            assert(*edge.raw != "-O2");
            assert(*edge.raw != "-g");
        }

        for (const auto &src : edge.sources)
        {
            assert(src.system != "vcpkg");
            assert(src.ref.find("guess:") == std::string::npos);
        }
    }

    assert(saw_expected_library_fragment);

    fs::remove_all(base);
}

int main()
{
    test_ingest_does_not_emit_guess_sources();
    std::cout << "[unit] file_api_no_guessing: OK\n";
    return 0;
}
