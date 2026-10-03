#pragma once
// Thin wrapper around Vallado's reference SGP4: one Propagator per object.
//
// Output is position/velocity in the TEME frame (km, km/s). Every object
// shares that frame at a given instant, so distances between objects need no
// frame conversion.

#include "kessler/sgp4_elements.hpp"
#include "kessler/time.hpp"

#include <array>
#include <stdexcept>
#include <string>

#include "SGP4.h"
// Vallado's header defines `pi` as a macro, which would break any later code
// that uses std::numbers::pi. Nothing of ours needs the macro, so remove it.
#ifdef pi
#undef pi
#endif

namespace kessler {

struct StateVector {
    std::array<double, 3> r_km{};    // position [km], TEME
    std::array<double, 3> v_km_s{};  // velocity [km/s], TEME
};

// 'i' = improved mode (Vallado's recommendation; python-sgp4's default).
// 'a' = reproduce the original Air Force (AFSPC) code exactly.
enum class Sgp4Mode : char { improved = 'i', afspc = 'a' };

struct Sgp4InitError : std::runtime_error {
    int code;
    Sgp4InitError(const std::string& msg, int c) : std::runtime_error(msg), code(c) {}
};

// SGP4 error codes (see the header comments in Vallado's SGP4.cpp):
//   1 mean eccentricity out of range or semi-major axis too small
//   2 mean motion < 0       3 perturbed eccentricity out of range
//   4 semi-latus rectum < 0 6 satellite has decayed
class Propagator {
public:
    /// Throws Sgp4InitError if SGP4 rejects the elements.
    explicit Propagator(const Sgp4Elements& elements, Sgp4Mode mode = Sgp4Mode::improved);

    /// Propagates to `minutes_since_epoch` (negative = before the epoch).
    /// Returns false if SGP4 reports an error; error_code() then says which.
    bool propagate(double minutes_since_epoch, StateVector& out);

    /// Propagates to an absolute UTC instant.
    bool propagate_to(JulianDate t, StateVector& out) {
        return propagate(minutes_between(epoch_, t), out);
    }

    [[nodiscard]] int error_code() const { return rec_.error; }
    [[nodiscard]] const JulianDate& epoch() const { return epoch_; }
    [[nodiscard]] int catalog_number() const { return catalog_number_; }

private:
    elsetrec rec_{};
    JulianDate epoch_{};
    int catalog_number_{};
};

} // namespace kessler
