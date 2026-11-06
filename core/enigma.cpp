// Enigma cryptographic core implementation.
// Signal order (strictly as on the real machine):
//   key press -> plugboard -> ETW -> rotors (right to left) -> reflector
//             -> rotors (left to right) -> ETW -> plugboard -> lampboard

#include "enigma.hpp"

#include "wiring.hpp"

#include <array>
#include <string>
#include <vector>

namespace enigma::core {
namespace {

const RotorSpec kRotorTable[] = {
    {"I",     kWiringRotorI,     kNotchRotorI,    false},
    {"II",    kWiringRotorII,    kNotchRotorII,   false},
    {"III",   kWiringRotorIII,   kNotchRotorIII,  false},
    {"IV",    kWiringRotorIV,    kNotchRotorIV,   false},
    {"V",     kWiringRotorV,     kNotchRotorV,    false},
    {"VI",    kWiringRotorVI,    kNotchRotorVI,   false},
    {"VII",   kWiringRotorVII,   kNotchRotorVII,  false},
    {"VIII",  kWiringRotorVIII,  kNotchRotorVIII, false},
    {"Beta",  kWiringRotorBeta,  kNotchNone,      true},
    {"Gamma", kWiringRotorGamma, kNotchNone,      true},
};

const ReflectorSpec kReflectorTable[] = {
    {"UKW-A",      kWiringReflectorA,     false},
    {"UKW-B",      kWiringReflectorB,     false},
    {"UKW-C",      kWiringReflectorC,     false},
    {"UKW-B-thin", kWiringReflectorBThin, true},
    {"UKW-C-thin", kWiringReflectorCThin, true},
};

bool isEnigmaIRotor(const RotorSpec* spec) {
    // Enigma I carries only rotors I..V.
    static const char* kEnigmaIRotorNames[] = {"I", "II", "III", "IV", "V"};
    for (const char* name : kEnigmaIRotorNames) {
        if (spec->name == name) {
            return true;
        }
    }
    return false;
}

std::array<int, kAlphabetSize> compileWiring(std::string_view wiring,
                                             std::string_view context) {
    std::array<int, kAlphabetSize> table{};
    std::array<bool, kAlphabetSize> seen{};
    if (wiring.size() != static_cast<size_t>(kAlphabetSize)) {
        throw ConfigError("core: wiring for '" + std::string(context) +
                          "' must contain exactly 26 letters");
    }
    for (int i = 0; i < kAlphabetSize; ++i) {
        const char c = wiring[static_cast<size_t>(i)];
        if (!isUppercaseLetter(c) || seen[static_cast<size_t>(c - 'A')]) {
            throw ConfigError("core: wiring for '" + std::string(context) +
                              "' is not a permutation of A..Z");
        }
        seen[static_cast<size_t>(c - 'A')] = true;
        table[static_cast<size_t>(i)] = c - 'A';
    }
    return table;
}

}  // namespace

int rotorCountFor(MachineType type) {
    return type == MachineType::M4 ? kMaxRotors : kMinRotors;
}

std::span<const RotorSpec> allRotors() {
    return std::span<const RotorSpec>(kRotorTable);
}

std::span<const ReflectorSpec> allReflectors() {
    return std::span<const ReflectorSpec>(kReflectorTable);
}

const RotorSpec* findRotor(std::string_view name) {
    for (const RotorSpec& spec : kRotorTable) {
        if (spec.name == name) {
            return &spec;
        }
    }
    return nullptr;
}

const ReflectorSpec* findReflector(std::string_view name) {
    for (const ReflectorSpec& spec : kReflectorTable) {
        if (spec.name == name) {
            return &spec;
        }
    }
    return nullptr;
}

int Machine::letterToIndex(char c) {
    if (!isUppercaseLetter(c)) {
        throw std::invalid_argument("core: expected uppercase letter A..Z");
    }
    return c - kFirstLetter;
}

char Machine::indexToLetter(int index) {
    if (index < 0 || index >= kAlphabetSize) {
        throw std::invalid_argument("core: letter index out of range");
    }
    return static_cast<char>(kFirstLetter + index);
}

void MachineConfig::validate() const {
    const int expectedRotors = rotorCountFor(type);
    if (static_cast<int>(rotors.size()) != expectedRotors) {
        throw ConfigError("core: machine type requires " +
                          std::to_string(expectedRotors) + " rotors, got " +
                          std::to_string(rotors.size()));
    }

    for (size_t i = 0; i < rotors.size(); ++i) {
        const RotorSetting& setting = rotors[i];
        const RotorSpec* spec = findRotor(setting.rotor);
        if (spec == nullptr) {
            throw ConfigError("core: unknown rotor '" + setting.rotor + "'");
        }
        if (!isUppercaseLetter(setting.ring)) {
            throw ConfigError("core: ring setting must be A..Z (rotor slot " +
                              std::to_string(i + 1) + ")");
        }
        if (!isUppercaseLetter(setting.position)) {
            throw ConfigError("core: start position must be A..Z (rotor slot " +
                              std::to_string(i + 1) + ")");
        }
        // No rotor may be fitted twice.
        for (size_t j = i + 1; j < rotors.size(); ++j) {
            if (rotors[j].rotor == setting.rotor) {
                throw ConfigError("core: rotor '" + setting.rotor +
                                  "' is fitted more than once");
            }
        }
        switch (type) {
            case MachineType::EnigmaI:
                if (!isEnigmaIRotor(spec)) {
                    throw ConfigError("core: Enigma I only accepts rotors I..V, got '" +
                                      setting.rotor + "'");
                }
                break;
            case MachineType::M3:
                if (spec->thin) {
                    throw ConfigError("core: thin rotors are not used on M3");
                }
                break;
            case MachineType::M4:
                // Thin rotor goes in the leftmost slot only, and that slot
                // must hold a thin rotor.
                if (spec->thin != (i == 0)) {
                    throw ConfigError("core: M4 requires a thin rotor (Beta/Gamma) "
                                      "in the leftmost slot only");
                }
                break;
        }
    }

    const ReflectorSpec* reflectorSpec = findReflector(reflector);
    if (reflectorSpec == nullptr) {
        throw ConfigError("core: unknown reflector '" + reflector + "'");
    }
    if (type == MachineType::M4 && !reflectorSpec->thin) {
        throw ConfigError("core: M4 requires a thin reflector (UKW-B-thin / UKW-C-thin)");
    }
    if (type != MachineType::M4 && reflectorSpec->thin) {
        throw ConfigError("core: thin reflectors are only used on M4");
    }

    if (plugboard.size() > static_cast<size_t>(kMaxPlugboardPairs)) {
        throw ConfigError("core: plugboard supports at most 13 pairs, got " +
                          std::to_string(plugboard.size()));
    }
    std::array<bool, kAlphabetSize> used{};
    for (const PlugboardPair& pair : plugboard) {
        if (!isUppercaseLetter(pair.first) || !isUppercaseLetter(pair.second)) {
            throw ConfigError("core: plugboard letters must be A..Z");
        }
        const int a = pair.first - kFirstLetter;
        const int b = pair.second - kFirstLetter;
        if (a == b) {
            throw ConfigError("core: plugboard pair cannot connect a letter to itself");
        }
        if (used[static_cast<size_t>(a)] || used[static_cast<size_t>(b)]) {
            throw ConfigError("core: plugboard letter '" +
                              std::string(1, used[static_cast<size_t>(a)] ? pair.first : pair.second) +
                              "' is used more than once");
        }
        used[static_cast<size_t>(a)] = true;
        used[static_cast<size_t>(b)] = true;
    }
}

bool Machine::Rotor::atNotch() const {
    if (spec->notches.empty()) {
        return false;  // thin rotors never step
    }
    const char positionLetter = indexToLetter(position);
    return spec->notches.find(positionLetter) != std::string_view::npos;
}

Machine::Machine(const MachineConfig& config) : config_(config) {
    config_.validate();

    rotors_.reserve(config_.rotors.size());
    for (const RotorSetting& setting : config_.rotors) {
        const RotorSpec* spec = findRotor(setting.rotor);
        Rotor rotor;
        rotor.spec = spec;
        rotor.forward = compileWiring(spec->wiring, spec->name);
        for (int i = 0; i < kAlphabetSize; ++i) {
            rotor.inverse[static_cast<size_t>(rotor.forward[static_cast<size_t>(i)])] = i;
        }
        rotor.ring = letterToIndex(setting.ring);
        rotor.position = letterToIndex(setting.position);
        rotors_.push_back(rotor);
    }

    const ReflectorSpec* reflectorSpec = findReflector(config_.reflector);
    reflector_ = compileWiring(reflectorSpec->wiring, reflectorSpec->name);

    for (int i = 0; i < kAlphabetSize; ++i) {
        etw_[static_cast<size_t>(i)] = i;
        plugboard_[static_cast<size_t>(i)] = i;
    }
    buildPlugboard();

    for (size_t i = 0; i < rotors_.size(); ++i) {
        initialPositions_[i] = rotors_[i].position;
    }
    keypressCount_ = 0;
}

void Machine::buildPlugboard() {
    for (const PlugboardPair& pair : config_.plugboard) {
        const int a = letterToIndex(pair.first);
        const int b = letterToIndex(pair.second);
        plugboard_[static_cast<size_t>(a)] = b;
        plugboard_[static_cast<size_t>(b)] = a;
    }
}

void Machine::step() {
    // Real pawl behaviour, evaluated on the positions BEFORE this keypress:
    //  - right rotor steps on every keypress;
    //  - if the middle rotor is on one of its notches, this keypress also
    //    advances the middle rotor and the rotor to its left (the
    //    double-stepping anomaly);
    //  - otherwise, if the right rotor is on its notch, the middle rotor
    //    advances too (it is carried by the right rotor's pawl).
    // Thin rotors never step.
    const int count = static_cast<int>(rotors_.size());
    const int right = count - 1;
    const int middle = count - 2;
    const int left = middle - 1;

    const bool middleAtNotch = rotors_[static_cast<size_t>(middle)].atNotch();
    const bool rightAtNotch = rotors_[static_cast<size_t>(right)].atNotch();

    Rotor& rightRotor = rotors_[static_cast<size_t>(right)];
    rightRotor.position = (rightRotor.position + 1) % kAlphabetSize;

    if (middleAtNotch) {
        Rotor& middleRotor = rotors_[static_cast<size_t>(middle)];
        middleRotor.position = (middleRotor.position + 1) % kAlphabetSize;
        Rotor& leftRotor = rotors_[static_cast<size_t>(left)];
        leftRotor.position = (leftRotor.position + 1) % kAlphabetSize;
    } else if (rightAtNotch) {
        Rotor& middleRotor = rotors_[static_cast<size_t>(middle)];
        middleRotor.position = (middleRotor.position + 1) % kAlphabetSize;
    }

    ++keypressCount_;
}

char Machine::encipher(char c) {
    step();
    return encipherWithoutStep(c);
}

char Machine::encipherWithoutStep(char c) const {
    int s = letterToIndex(c);

    s = plugboard_[static_cast<size_t>(s)];
    s = etw_[static_cast<size_t>(s)];  // entry wheel (identity on I / M3 / M4)

    // Right to left through the rotors.
    for (int i = static_cast<int>(rotors_.size()) - 1; i >= 0; --i) {
        const Rotor& rotor = rotors_[static_cast<size_t>(i)];
        const int contact =
            (s + rotor.position - rotor.ring + kAlphabetSize) % kAlphabetSize;
        s = (rotor.forward[static_cast<size_t>(contact)] - rotor.position +
             rotor.ring + kAlphabetSize) %
            kAlphabetSize;
    }

    s = reflector_[static_cast<size_t>(s)];

    // Left to right back through the rotors (inverse permutations).
    for (const Rotor& rotor : rotors_) {
        const int contact =
            (s + rotor.position - rotor.ring + kAlphabetSize) % kAlphabetSize;
        s = (rotor.inverse[static_cast<size_t>(contact)] - rotor.position +
             rotor.ring + kAlphabetSize) %
            kAlphabetSize;
    }

    s = etw_[static_cast<size_t>(s)];
    s = plugboard_[static_cast<size_t>(s)];
    return indexToLetter(s);
}

void Machine::reset() {
    for (size_t i = 0; i < rotors_.size(); ++i) {
        rotors_[i].position = initialPositions_[i];
    }
    keypressCount_ = 0;
}

std::string Machine::positions() const {
    std::string result;
    result.reserve(rotors_.size());
    for (const Rotor& rotor : rotors_) {
        result.push_back(indexToLetter(rotor.position));
    }
    return result;
}

}  // namespace enigma::core
