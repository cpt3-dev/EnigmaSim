#pragma once
// Historical Enigma wiring tables, transcribed verbatim (A = 0).
// These strings are ground truth: never "recall", shorten, or alter them.

#include <string_view>

namespace enigma::core {

inline constexpr std::string_view kNotchNone{};

inline constexpr std::string_view kWiringRotorI     = "EKMFLGDQVZNTOWYHXUSPAIBRCJ";  // notch Q
inline constexpr std::string_view kNotchRotorI      = "Q";
inline constexpr std::string_view kWiringRotorII    = "AJDKSIRUXBLHWTMCQGZNPYFVOE";  // notch E
inline constexpr std::string_view kNotchRotorII     = "E";
inline constexpr std::string_view kWiringRotorIII   = "BDFHJLCPRTXVZNYEIWGAKMUSQO";  // notch V
inline constexpr std::string_view kNotchRotorIII    = "V";
inline constexpr std::string_view kWiringRotorIV    = "ESOVPZJAYQUIRHXLNFTGKDCMWB";  // notch J
inline constexpr std::string_view kNotchRotorIV     = "J";
inline constexpr std::string_view kWiringRotorV     = "VZBRGITYUPSDNHLXAWMJQOFECK";  // notch Z
inline constexpr std::string_view kNotchRotorV      = "Z";
inline constexpr std::string_view kWiringRotorVI    = "JPGVOUMFYQBENHZRDKASXLICTW";  // notches Z, M
inline constexpr std::string_view kNotchRotorVI     = "ZM";
inline constexpr std::string_view kWiringRotorVII   = "NZJHGRCXMYSWBOUFAIVLPEKQDT";  // notches Z, M
inline constexpr std::string_view kNotchRotorVII    = "ZM";
inline constexpr std::string_view kWiringRotorVIII  = "FKQHTLXOCBJSPDZRAMEWNIUYGV";  // notches Z, M
inline constexpr std::string_view kNotchRotorVIII   = "ZM";
inline constexpr std::string_view kWiringRotorBeta  = "LEYJVCNIXWPBQMDRTAKZGFUHOS";  // thin rotor, never steps
inline constexpr std::string_view kWiringRotorGamma = "FSOKANUERHMBTIYCWLQPZXVGJD";  // thin rotor, never steps

inline constexpr std::string_view kWiringReflectorA     = "EJMZALYXVBWFCRQUONTSPIKHGD";
inline constexpr std::string_view kWiringReflectorB     = "YRUHQSLDPXNGOKMIEBFZCWVJAT";
// Historical UKW-C wiring. NOTE: the source specification table listed a
// 28-character line (with duplicated 'A' and 'Z'), which is not a valid
// permutation; the historically correct wiring is used instead.
inline constexpr std::string_view kWiringReflectorC     = "FVPJIAOYEDRZXWGCTKUQSBNMHL";
inline constexpr std::string_view kWiringReflectorBThin = "EJMZALYXVBWFCRQUONTSPIKHGD";
inline constexpr std::string_view kWiringReflectorCThin = "YRUHQSLDPXNGOKMIEBFZCWVJAT";

inline constexpr std::string_view kWiringEtw = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

}  // namespace enigma::core
