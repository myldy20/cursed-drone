// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "cursed_drone/audio.hpp"
#include "cursed_drone/session.hpp"

#include <memory>

namespace cursed_drone::detail {

class PlaitsActor {
public:
    PlaitsActor();
    ~PlaitsActor();
    PlaitsActor(const PlaitsActor&) = delete;
    PlaitsActor& operator=(const PlaitsActor&) = delete;

    void prepare(float sample_rate);
    void reset() noexcept;
    [[nodiscard]] StereoFrame next(
        PlaitsModel model,
        PlaitsOutputMode output_mode,
        const ScalaTuning& tuning,
        float frequency_hz,
        float harmonics,
        float timbre,
        float morph,
        float decay,
        bool trigger,
        bool trigger_patched) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace cursed_drone::detail
