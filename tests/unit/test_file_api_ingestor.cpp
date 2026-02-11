#include "depbridge/ingest/cmake/file_api_ingestor.hpp"

#include <cassert>
#include <iostream>
#include <vector>

static void test_deterministic_index_selection_is_stable()
{
    using depbridge::ingest::cmake::select_deterministic_file_api_index;

    std::vector<std::filesystem::path> a = {
        "/tmp/reply/index-aaa.json",
        "/tmp/reply/index-zzz.json",
        "/tmp/reply/index-mid.json",
    };
    std::vector<std::filesystem::path> b = {
        "/tmp/reply/index-mid.json",
        "/tmp/reply/index-aaa.json",
        "/tmp/reply/index-zzz.json",
    };

    const auto chosen_a = select_deterministic_file_api_index(a);
    const auto chosen_b = select_deterministic_file_api_index(b);

    assert(chosen_a.filename() == "index-zzz.json");
    assert(chosen_b.filename() == "index-zzz.json");
}

int main()
{
    test_deterministic_index_selection_is_stable();
    std::cout << "[unit] file_api_ingestor: OK\n";
    return 0;
}
