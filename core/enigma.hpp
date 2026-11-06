#pragma once
// Public interface of the Enigma cryptographic core.
// Pure C++20: this header must never include windows.h or any OpenGL header.

#include <array>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace enigma::core {

constexpr int kAlphabetSize = 26;
constexpr int kMaxPlugboardPairs = 13;
constexpr int kMinRotors = 3;
constexpr int kMaxRotors = 4;
constexpr char kFirstLetter = 'A';

[[nodiscard]] constexpr bool isUppercaseLetter(char c) {
    return c >= 'A' && c <= 'Z';
}

// Thrown for any invalid machine configuration. Never silently corrected.
class ConfigError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

enum class MachineType : int { EnigmaI = 0, M3 = 1, M4 = 2 };

// Number of rotor slots for a machine type (3 for Enigma I / M3, 4 for M4).
[[nodiscard]] int rotorCountFor(MachineType type);

struct RotorSpec {
    std::string_view name;
    std::string_view wiring;   // 26 uppercase letters, forward permutation
    std::string_view notches;  // turnover letters; empty for thin rotors
    bool thin;                 // thin rotors (Beta/Gamma) never step
};

struct ReflectorSpec {
    std::string_view name;
    std::string_view wiring;
    bool thin;
};

[[nodiscard]] std::span<const RotorSpec> allRotors();
[[nodiscard]] std::span<const ReflectorSpec> allReflectors();
[[nodiscard]] const RotorSpec* findRotor(std::string_view name);
[[nodiscard]] const ReflectorSpec* findReflector(std::string_view name);

struct RotorSetting {
    std::string rotor;      // spec name, e.g. "III" or "Beta"
    char ring = 'A';        // Ringstellung, 'A'..'Z'
    char position = 'A';    // ground position, 'A'..'Z'
};

struct PlugboardPair {
    char first = 'A';
    char second = 'B';
};

struct MachineConfig {
    MachineType type = MachineType::EnigmaI;
    std::vector<RotorSetting> rotors;  // ordered left -> right
    std::string reflector = "UKW-B";
    std::vector<PlugboardPair> plugboard;  // 0..13 pairs

    [[nodiscard]] bool operator==(const MachineConfig&) const = default;

    // Throws ConfigError on any invalid field.
    void validate() const;
};

// A configured Enigma machine. Positions change before each enciphered
// character (stepping happens BEFORE enciphering, as on the real hardware).
class Machine {
public:
    // Throws ConfigError if the configuration is invalid.
    explicit Machine(const MachineConfig& config);

    [[nodiscard]] const MachineConfig& config() const { return config_; }

    // Steps the rotors per the real rules (including the double-stepping
    // anomaly), then enciphers c. c must be 'A'..'Z'.
    [[nodiscard]] char encipher(char c);

    // Enciphers c WITHOUT stepping. Used by animation code that renders
    // rotor stepping as a separate visual stage.
    [[nodiscard]] char encipherWithoutStep(char c) const;

    // Advances the rotor positions by one keypress (real pawl rules).
    void step();

    // Restores the configured ground positions and clears the key counter.
    void reset();

    [[nodiscard]] std::string positions() const;  // left -> right, e.g. "AAA"
    [[nodiscard]] int keypressCount() const { return keypressCount_; }

    // Converts 'A'..'Z' to 0..25. Throws std::invalid_argument otherwise.
    [[nodiscard]] static int letterToIndex(char c);
    // Converts 0..25 to 'A'..'Z'. Throws std::invalid_argument otherwise.
    [[nodiscard]] static char indexToLetter(int index);

private:
    struct Rotor {
        const RotorSpec* spec = nullptr;
        std::array<int, kAlphabetSize> forward{};
        std::array<int, kAlphabetSize> inverse{};
        int ring = 0;
        int position = 0;

        [[nodiscard]] bool atNotch() const;
    };

    void buildPlugboard();

    MachineConfig config_;
    std::vector<Rotor> rotors_;                   // left -> right
    std::array<int, kAlphabetSize> reflector_{};
    std::array<int, kAlphabetSize> etw_{};        // entry wheel (identity here)
    std::array<int, kAlphabetSize> plugboard_{};  // identity where unpaired
    std::array<int, kMaxRotors> initialPositions_{};
    int keypressCount_ = 0;
};

}  // namespace enigma::core
