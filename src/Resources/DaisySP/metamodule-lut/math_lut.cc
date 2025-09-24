#include "math_lut.hh"
#include <cmath>

struct Pow2TableRange {
	static constexpr float min = -5.1f;
	static constexpr float max = 6.1f;
};

Mapping::LookupTable_t<64, float> Pow2 =
	Mapping::LookupTable_t<64, float>::generate<Pow2TableRange>([](auto x) { return std::pow(2.f, x); });

struct Pow_025_TableRange {
	static constexpr float min = 0.f;
	static constexpr float max = 1.f;
};

Mapping::LookupTable_t<64, float> Pow_025 =
	Mapping::LookupTable_t<64, float>::generate<Pow_025_TableRange>([](auto x) { return std::pow(x, 0.25f); });

struct SinfTableRange {
	static constexpr float min = 0.f;
	static constexpr float max = 3.1416f * 2.f;
};
Mapping::LookupTable_t<64, float> Sinf =
	Mapping::LookupTable_t<64, float>::generate<SinfTableRange>([](auto x) { return std::sin(x); });

struct CosfTableRange {
	static constexpr float min = 0.f;
	static constexpr float max = 3.1416f * 2.f;
};
Mapping::LookupTable_t<64, float> Cosf =
	Mapping::LookupTable_t<64, float>::generate<CosfTableRange>([](auto x) { return std::cos(x); });
