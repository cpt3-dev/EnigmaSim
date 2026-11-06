// Acceptance tests for the Enigma cryptographic core.
//
// Expected values in the "custom vector" tests were derived by hand from the
// wiring tables (independent of the implementation) and cover:
//   - plugboard in both directions, non-zero ring settings,
//   - a double-stepping trigger (middle rotor on its notch),
//   - the M4 4-rotor path with a thin rotor and thin reflector.

#include "test_framework.hpp"

#include "enigma.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

using namespace enigma::core;

namespace {

MachineConfig enigmaI_ABC() {
    // Enigma I, rotors I-II-III, UKW-B, rings AAA, start AAA, no plugboard.
    MachineConfig cfg;
    cfg.type = MachineType::EnigmaI;
    cfg.rotors = {{"I", 'A', 'A'}, {"II", 'A', 'A'}, {"III", 'A', 'A'}};
    cfg.reflector = "UKW-B";
    cfg.plugboard.clear();
    return cfg;
}

std::string encrypt(const MachineConfig& cfg, std::string_view text) {
    Machine machine(cfg);
    std::string out;
    for (const char c : text) {
        out.push_back(machine.encipher(c));
    }
    return out;
}

template <typename Fn>
bool throwsConfigError(Fn&& fn) {
    try {
        fn();
    } catch (const ConfigError&) {
        return true;
    }
    return false;
}

}  // namespace

TEST(literature_vector_aaaaa_bdzgo) {
    // Published vector: Enigma I, I-II-III, UKW-B, AAA/AAA, no plugboard.
    CHECK_EQ(encrypt(enigmaI_ABC(), "AAAAA"), "BDZGO");
    // Fresh machine for the 10-character vector (same initial state).
    CHECK_EQ(encrypt(enigmaI_ABC(), "AAAAAAAAAA"), "BDZGOWCXLT");
}

TEST(custom_vector_plugboard_and_rings) {
    // Hand-verified: rotors I-II-III, UKW-B, rings B-A-C, positions AAA,
    // plugboard A<->B. Stepping moves the right rotor to B first.
    // Path: A -plug-> B -> III -> II -> I -> UKW-B -> I -> II -> III
    //       -plug-> (N unpaired) = N.
    MachineConfig cfg = enigmaI_ABC();
    cfg.rotors = {{"I", 'B', 'A'}, {"II", 'A', 'A'}, {"III", 'C', 'A'}};
    cfg.plugboard = {{'A', 'B'}};
    CHECK_EQ(encrypt(cfg, "A"), "N");
}

TEST(custom_vector_double_step) {
    // Hand-verified: rotors I-II-III, UKW-B, rings AAA, positions AEW.
    // The middle rotor (II) sits on its notch E, so this keypress performs a
    // double step: left A->B, middle E->F, right W->X, then enciphers 'A'
    // to 'W'.
    MachineConfig cfg = enigmaI_ABC();
    cfg.rotors = {{"I", 'A', 'A'}, {"II", 'A', 'E'}, {"III", 'A', 'W'}};
    cfg.plugboard = {{'A', 'B'}};
    Machine machine(cfg);
    CHECK_EQ(machine.positions(), "AEW");
    CHECK_EQ(machine.encipher('A'), 'W');
    CHECK_EQ(machine.positions(), "BFX");
}

TEST(custom_vector_m4_thin_rotor) {
    // Hand-verified: M4, rotors Beta-I-II-III (left to right), thin UKW-B,
    // rings A-A-B-C, positions AAAA, plugboard A<->B. 'A' enciphers to 'I'
    // through the thin rotor and thin reflector path.
    MachineConfig cfg;
    cfg.type = MachineType::M4;
    cfg.rotors = {{"Beta", 'A', 'A'}, {"I", 'A', 'A'}, {"II", 'B', 'A'}, {"III", 'C', 'A'}};
    cfg.reflector = "UKW-B-thin";
    cfg.plugboard = {{'A', 'B'}};
    CHECK_EQ(encrypt(cfg, "A"), "I");
}

TEST(stepping_double_step_sequence) {
    // Canonical pawl sequence with rotors I-II-III (notches Q/E/V):
    // ADU -> ADV -> AEW -> BFX.
    MachineConfig cfg = enigmaI_ABC();
    cfg.rotors = {{"I", 'A', 'A'}, {"II", 'A', 'D'}, {"III", 'A', 'U'}};
    Machine machine(cfg);
    static_cast<void>(machine.encipher('A'));
    CHECK_EQ(machine.positions(), "ADV");  // right rotor carried middle past notch V
    static_cast<void>(machine.encipher('A'));
    CHECK_EQ(machine.positions(), "AEW");
    static_cast<void>(machine.encipher('A'));
    CHECK_EQ(machine.positions(), "BFX");  // double step: middle E -> left and middle advance
}

TEST(dual_notch_rotors) {
    // Rotors VI/VII/VIII have two notches (Z and M); either must trigger the
    // double step. Middle rotor VIII, notches ZM.
    MachineConfig cfg = enigmaI_ABC();
    cfg.type = MachineType::M3;
    cfg.rotors = {{"I", 'A', 'A'}, {"VIII", 'A', 'Z'}, {"III", 'A', 'A'}};
    Machine machine(cfg);
    static_cast<void>(machine.encipher('A'));
    CHECK_EQ(machine.positions(), "BAB");  // left and middle stepped together

    Machine machineAtM(cfg);
    machineAtM.reset();
    CHECK_EQ(machineAtM.positions(), "AZA");
    // Move middle to its second notch 'M' by direct configuration instead:
    MachineConfig cfgAtM = cfg;
    cfgAtM.rotors = {{"I", 'A', 'A'}, {"VIII", 'A', 'M'}, {"III", 'A', 'A'}};
    Machine machineM(cfgAtM);
    static_cast<void>(machineM.encipher('A'));
    CHECK_EQ(machineM.positions(), "BNB");  // middle M -> N

    // Control: middle NOT on a notch -> only the right rotor steps.
    MachineConfig cfgControl = cfg;
    cfgControl.rotors = {{"I", 'A', 'A'}, {"VIII", 'A', 'Y'}, {"III", 'A', 'A'}};
    Machine machineControl(cfgControl);
    static_cast<void>(machineControl.encipher('A'));
    CHECK_EQ(machineControl.positions(), "AYB");
}

TEST(thin_rotor_never_steps) {
    MachineConfig cfg;
    cfg.type = MachineType::M4;
    cfg.rotors = {{"Gamma", 'A', 'A'}, {"I", 'A', 'A'}, {"II", 'A', 'A'}, {"III", 'A', 'A'}};
    cfg.reflector = "UKW-C-thin";
    Machine machine(cfg);
    for (int i = 0; i < 30; ++i) {
        static_cast<void>(machine.encipher('A'));
    }
    CHECK_EQ(machine.positions().front(), 'A');  // thin rotor never moves
    CHECK_EQ(machine.keypressCount(), 30);
}

TEST(reflexivity) {
    // Re-enciphering the ciphertext from the same initial state returns the
    // plaintext (decrypt = encrypt on the Enigma).
    MachineConfig cfg = enigmaI_ABC();
    cfg.plugboard = {{'A', 'M'}, {'T', 'E'}, {'N', 'W'}};
    cfg.rotors = {{"IV", 'D', 'K'}, {"II", 'B', 'P'}, {"V", 'Z', 'F'}};
    const std::string plaintext = "ATTACKATDAWN";
    Machine machine(cfg);
    std::string ciphertext;
    for (const char c : plaintext) {
        ciphertext.push_back(machine.encipher(c));
    }
    machine.reset();
    std::string recovered;
    for (const char c : ciphertext) {
        recovered.push_back(machine.encipher(c));
    }
    CHECK_EQ(recovered, plaintext);
}

TEST(no_letter_enciphers_to_itself) {
    // Under any configuration no letter can ever encipher to itself.
    std::vector<MachineConfig> configs;
    configs.push_back(enigmaI_ABC());

    MachineConfig plugged = enigmaI_ABC();
    plugged.plugboard = {{'B', 'C'}, {'D', 'E'}, {'F', 'G'}, {'H', 'I'}};
    configs.push_back(plugged);

    MachineConfig rings = enigmaI_ABC();
    rings.rotors = {{"I", 'Q', 'W'}, {"II", 'E', 'R'}, {"III", 'T', 'Y'}};
    configs.push_back(rings);

    MachineConfig m4;
    m4.type = MachineType::M4;
    m4.rotors = {{"Beta", 'D', 'A'}, {"VIII", 'A', 'C'}, {"VI", 'M', 'B'}, {"V", 'A', 'X'}};
    m4.reflector = "UKW-B-thin";
    m4.plugboard = {{'Q', 'W'}};
    configs.push_back(m4);

    for (const MachineConfig& cfg : configs) {
        Machine machine(cfg);
        for (char c = 'A'; c <= 'Z'; ++c) {
            machine.reset();
            CHECK(machine.encipher(c) != c);
        }
    }
}

TEST(wiring_tables_are_permutations) {
    for (const RotorSpec& spec : allRotors()) {
        CHECK_EQ(spec.wiring.size(), size_t{26});
        bool used[kAlphabetSize] = {};
        for (const char c : spec.wiring) {
            CHECK(isUppercaseLetter(c));
            if (isUppercaseLetter(c)) {
                CHECK(!used[c - 'A']);
                used[c - 'A'] = true;
            }
        }
    }
    for (const ReflectorSpec& spec : allReflectors()) {
        CHECK_EQ(spec.wiring.size(), size_t{26});
        bool used[kAlphabetSize] = {};
        for (const char c : spec.wiring) {
            CHECK(isUppercaseLetter(c));
            if (isUppercaseLetter(c)) {
                CHECK(!used[c - 'A']);
                used[c - 'A'] = true;
            }
        }
    }
}

TEST(config_error_plugboard) {
    CHECK(throwsConfigError([] {
        MachineConfig cfg = enigmaI_ABC();
        cfg.plugboard = {{'A', 'B'}, {'B', 'C'}};  // B reused
        Machine machine(cfg);
        static_cast<void>(machine);
    }));
    CHECK(throwsConfigError([] {
        MachineConfig cfg = enigmaI_ABC();
        for (int i = 0; i < kMaxPlugboardPairs + 1; ++i) {
            cfg.plugboard.push_back(
                {static_cast<char>('A' + 2 * i), static_cast<char>('A' + 2 * i + 1)});
        }
        Machine machine(cfg);  // 14 pairs
        static_cast<void>(machine);
    }));
    CHECK(throwsConfigError([] {
        MachineConfig cfg = enigmaI_ABC();
        cfg.plugboard = {{'A', 'A'}};  // self-paired
        Machine machine(cfg);
        static_cast<void>(machine);
    }));
    CHECK(throwsConfigError([] {
        MachineConfig cfg = enigmaI_ABC();
        cfg.plugboard = {{'a', 'B'}};  // not uppercase
        Machine machine(cfg);
        static_cast<void>(machine);
    }));
}

TEST(config_error_rotor_selection) {
    CHECK(throwsConfigError([] {
        MachineConfig cfg = enigmaI_ABC();
        cfg.rotors = {{"VI", 'A', 'A'}, {"II", 'A', 'A'}, {"III", 'A', 'A'}};
        Machine machine(cfg);  // rotor VI not available on Enigma I
        static_cast<void>(machine);
    }));
    CHECK(throwsConfigError([] {
        MachineConfig cfg = enigmaI_ABC();
        cfg.rotors = {{"I", 'A', 'A'}, {"I", 'A', 'A'}, {"III", 'A', 'A'}};
        Machine machine(cfg);  // rotor fitted twice
        static_cast<void>(machine);
    }));
    CHECK(throwsConfigError([] {
        MachineConfig cfg = enigmaI_ABC();
        cfg.rotors = {{"Beta", 'A', 'A'}, {"II", 'A', 'A'}, {"III", 'A', 'A'}};
        Machine machine(cfg);  // thin rotor on M3
        static_cast<void>(machine);
    }));
    CHECK(throwsConfigError([] {
        MachineConfig cfg;
        cfg.type = MachineType::M3;
        cfg.rotors = {{"I", 'A', 'A'}, {"II", 'A', 'A'}, {"III", 'A', 'A'}, {"IV", 'A', 'A'}};
        Machine machine(cfg);  // M3 takes exactly 3 rotors
        static_cast<void>(machine);
    }));
    CHECK(throwsConfigError([] {
        MachineConfig cfg;
        cfg.type = MachineType::M4;
        cfg.rotors = {{"I", 'A', 'A'}, {"II", 'A', 'A'}, {"III", 'A', 'A'}, {"IV", 'A', 'A'}};
        cfg.reflector = "UKW-B";  // not a thin reflector
        Machine machine(cfg);
        static_cast<void>(machine);
    }));
    CHECK(throwsConfigError([] {
        MachineConfig cfg;
        cfg.type = MachineType::M4;
        cfg.rotors = {{"Beta", 'A', 'A'}, {"II", 'A', 'A'}, {"Beta", 'A', 'A'}, {"IV", 'A', 'A'}};
        cfg.reflector = "UKW-B-thin";
        Machine machine(cfg);  // duplicate rotor
        static_cast<void>(machine);
    }));
    CHECK(throwsConfigError([] {
        MachineConfig cfg = enigmaI_ABC();
        cfg.rotors = {{"IX", 'A', 'A'}, {"II", 'A', 'A'}, {"III", 'A', 'A'}};
        Machine machine(cfg);  // unknown rotor name
        static_cast<void>(machine);
    }));
    CHECK(throwsConfigError([] {
        MachineConfig cfg = enigmaI_ABC();
        cfg.reflector = "UKW-D";
        Machine machine(cfg);  // unknown reflector name
        static_cast<void>(machine);
    }));
}

TEST(config_error_letter_fields) {
    CHECK(throwsConfigError([] {
        MachineConfig cfg = enigmaI_ABC();
        cfg.rotors = {{"I", '1', 'A'}, {"II", 'A', 'A'}, {"III", 'A', 'A'}};
        Machine machine(cfg);  // invalid ring letter
        static_cast<void>(machine);
    }));
    CHECK(throwsConfigError([] {
        MachineConfig cfg = enigmaI_ABC();
        cfg.rotors = {{"I", 'A', 'a'}, {"II", 'A', 'A'}, {"III", 'A', 'A'}};
        Machine machine(cfg);  // invalid start position
        static_cast<void>(machine);
    }));
    CHECK(throwsConfigError([] {
        try {
            static_cast<void>(Machine::letterToIndex('1'));
        } catch (const std::invalid_argument&) {
            throw ConfigError("expected");
        }
    }));
    CHECK(throwsConfigError([] {
        try {
            static_cast<void>(Machine::indexToLetter(26));
        } catch (const std::invalid_argument&) {
            throw ConfigError("expected");
        }
    }));
}

TEST(keypress_count_and_reset) {
    Machine machine(enigmaI_ABC());
    CHECK_EQ(machine.keypressCount(), 0);
    static_cast<void>(machine.encipher('A'));
    static_cast<void>(machine.encipher('B'));
    CHECK_EQ(machine.keypressCount(), 2);
    CHECK_EQ(machine.positions(), "AAC");
    machine.reset();
    CHECK_EQ(machine.keypressCount(), 0);
    CHECK_EQ(machine.positions(), "AAA");
}

int main() {
    return testfw::runAll();
}
