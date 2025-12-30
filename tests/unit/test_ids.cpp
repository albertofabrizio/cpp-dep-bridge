#include "depbridge/model/ids.hpp"
#include "depbridge/model/types.hpp"

#include <cassert>
#include <iostream>

using namespace depbridge::model;

static void test_component_id_purl_precedence() {
    Component a;
    a.type = ComponentType::library;
    a.name = "fmt";
    a.purl = "pkg:github/fmtlib/fmt@10.2.1";
    a.id = component_id_of(a);

    Component b = a;
    b.name = "something-else"; // should not matter if purl exists
    b.id = component_id_of(b);

    assert(a.id.value == b.id.value);
}

static void test_component_id_fields_affect_identity() {
    auto id1 = make_component_id(ComponentType::library, "ns", "foo", "1.0.0", "");
    auto id2 = make_component_id(ComponentType::library, "ns", "foo", "1.0.1", "");
    assert(id1.value != id2.value);

    auto id3 = make_component_id(ComponentType::library, "ns", "foo", "1.0.0", "");
    assert(id1.value == id3.value);
}

static void test_target_id_determinism() {
    auto a = make_target_id("cmake", "org/repo", "mytarget", "RelWithDebInfo");
    auto b = make_target_id("cmake", "org/repo", "mytarget", "RelWithDebInfo");
    auto c = make_target_id("cmake", "org/repo", "mytarget", "Debug");
    assert(a.value == b.value);
    assert(a.value != c.value);
}

static void test_canonical_key_stability() {
    const auto k1 = canonical_component_key(ComponentType::library, "ns", "foo", "1.0", "");
    const auto k2 = canonical_component_key(ComponentType::library, "ns", "foo", "1.0", "");
    assert(k1 == k2);

    const auto kt1 = canonical_target_key("cmake", "org/repo", "tgt", "Debug");
    const auto kt2 = canonical_target_key("cmake", "org/repo", "tgt", "Debug");
    assert(kt1 == kt2);
}

int main() {
    test_component_id_purl_precedence();
    test_component_id_fields_affect_identity();
    test_target_id_determinism();
    test_canonical_key_stability();

    std::cout << "[unit] ids: OK\n";
    return 0;
}
