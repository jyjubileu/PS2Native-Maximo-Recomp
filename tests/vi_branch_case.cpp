#include "hw.hpp"
#include "vif.hpp"
#include "vu_micro.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#ifdef PS2_TEST_GENERATED_VU
void register_generated_vu();
#endif

int main(int argc, char** argv) {
#ifdef PS2_TEST_GENERATED_VU
    register_generated_vu();
#endif
    if (argc < 2) {
        std::fputs("ps2vibranchtest stream.bin [qword ...]\n", stderr);
        return 2;
    }
    auto* file = std::fopen(argv[1], "rb");
    if (!file) return 2;
    std::vector<std::uint32_t> words;
    for (std::uint32_t word; std::fread(&word, sizeof word, 1, file) == 1;) words.push_back(word);
    std::fclose(file);
    if (words.empty()) return 2;
    ps2::vif::reset();
    ps2::vu::reset();
    ps2::vif::transfer_words(1, words.data(), static_cast<std::uint32_t>(words.size()));
    ps2::vif::flush(1);
    std::vector<unsigned> addresses{0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x45};
    if (argc > 2) {
        addresses.clear();
        for (int a = 2; a < argc; ++a) {
            char* end = nullptr;
            const auto address = std::strtoul(argv[a], &end, 16);
            if (!*argv[a] || *end || address >= 1024) return 2;
            addresses.push_back(static_cast<unsigned>(address));
        }
    }
    std::fprintf(stderr, "fast_interpreter_calls=%llu\n",
                 static_cast<unsigned long long>(ps2::vu::interpreted_calls()));
    for (unsigned address : addresses) {
        std::printf("%03X ", address);
        for (unsigned i = 0; i < 16; ++i) std::printf("%02x", unsigned(ps2::g_vu1_mem[address*16+i]));
        std::putchar('\n');
    }
}
