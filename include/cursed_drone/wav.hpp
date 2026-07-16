// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "cursed_drone/audio.hpp"

#include <filesystem>
#include <string>

namespace cursed_drone {

[[nodiscard]] bool write_wav_f32(
    const std::filesystem::path& path,
    BufferView<const StereoFrame> frames,
    unsigned sample_rate,
    std::string& error);

[[nodiscard]] bool render_session_to_wav(
    const Session& session,
    const std::filesystem::path& path,
    float seconds,
    std::string& error);

} // namespace cursed_drone
