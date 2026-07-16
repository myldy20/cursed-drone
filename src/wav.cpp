// SPDX-License-Identifier: GPL-3.0-or-later
#include "cursed_drone/wav.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <limits>
#include <vector>

namespace cursed_drone {
namespace {

void write_u16(std::ostream& output, std::uint16_t value) {
    const std::array bytes{
        static_cast<char>(value & 0xFFU),
        static_cast<char>((value >> 8U) & 0xFFU),
    };
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

void write_u32(std::ostream& output, std::uint32_t value) {
    const std::array bytes{
        static_cast<char>(value & 0xFFU),
        static_cast<char>((value >> 8U) & 0xFFU),
        static_cast<char>((value >> 16U) & 0xFFU),
        static_cast<char>((value >> 24U) & 0xFFU),
    };
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

} // namespace

bool write_wav_f32(
    const std::filesystem::path& path,
    std::span<const StereoFrame> frames,
    unsigned sample_rate,
    std::string& error) {
    constexpr std::uint16_t channels = 2U;
    constexpr std::uint16_t bits_per_sample = 32U;
    constexpr std::uint16_t bytes_per_frame = channels * (bits_per_sample / 8U);
    const auto data_size_wide = frames.size() * static_cast<std::size_t>(bytes_per_frame);
    if (data_size_wide > std::numeric_limits<std::uint32_t>::max()) {
        error = "WAV output is too large";
        return false;
    }
    const auto data_size = static_cast<std::uint32_t>(data_size_wide);

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        error = "cannot open WAV for writing: " + path.string();
        return false;
    }

    output.write("RIFF", 4);
    write_u32(output, 36U + data_size);
    output.write("WAVE", 4);
    output.write("fmt ", 4);
    write_u32(output, 16U);
    write_u16(output, 3U); // IEEE float
    write_u16(output, channels);
    write_u32(output, sample_rate);
    write_u32(output, sample_rate * bytes_per_frame);
    write_u16(output, bytes_per_frame);
    write_u16(output, bits_per_sample);
    output.write("data", 4);
    write_u32(output, data_size);
    output.write(
        reinterpret_cast<const char*>(frames.data()),
        static_cast<std::streamsize>(data_size));

    if (!output) {
        error = "failed while writing WAV: " + path.string();
        return false;
    }
    return true;
}

bool render_session_to_wav(
    const Session& session,
    const std::filesystem::path& path,
    float seconds,
    std::string& error) {
    constexpr unsigned sample_rate = 48'000U;
    if (!(seconds > 0.0F) || seconds > 600.0F) {
        error = "render duration must be between 0 and 600 seconds";
        return false;
    }
    const auto frame_count = static_cast<std::size_t>(seconds * static_cast<float>(sample_rate));
    std::vector<StereoFrame> frames(frame_count);
    AudioGraph graph;
    graph.prepare({static_cast<float>(sample_rate), 512U}, session);
    constexpr std::size_t block_size = 512U;
    for (std::size_t offset = 0; offset < frame_count; offset += block_size) {
        const auto count = std::min(block_size, frame_count - offset);
        graph.process(std::span<StereoFrame>{frames.data() + offset, count});
    }
    return write_wav_f32(path, frames, sample_rate, error);
}

} // namespace cursed_drone
