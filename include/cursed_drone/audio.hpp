// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "cursed_drone/session.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <type_traits>

namespace cursed_drone {

struct StereoFrame {
    float left{0.0F};
    float right{0.0F};
};

static_assert(sizeof(StereoFrame) == sizeof(float) * 2U);

struct AudioConfig {
    float sample_rate{48'000.0F};
    std::size_t max_block_frames{512};
};

struct AudioTelemetry {
    std::array<float, kSlotCount> slot_rms{};
    std::array<float, kSlotCount> slot_peak{};
    float master_rms{0.0F};
    float master_peak{0.0F};
    float pulse_phase{0.0F};
    float chaos_activity{0.0F};
};

template <typename T, std::size_t Capacity>
class SpscQueue {
    static_assert(Capacity >= 2);
    static_assert(std::is_trivially_copyable_v<T>);

public:
    [[nodiscard]] bool push(const T& value) noexcept {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto next = (head + 1U) % Capacity;
        if (next == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        values_[head] = value;
        head_.store(next, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool pop(T& value) noexcept {
        const auto tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) {
            return false;
        }
        value = values_[tail];
        tail_.store((tail + 1U) % Capacity, std::memory_order_release);
        return true;
    }

private:
    std::array<T, Capacity> values_{};
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
};

class AudioGraph {
public:
    AudioGraph();
    ~AudioGraph();
    AudioGraph(AudioGraph&&) noexcept;
    AudioGraph& operator=(AudioGraph&&) noexcept;
    AudioGraph(const AudioGraph&) = delete;
    AudioGraph& operator=(const AudioGraph&) = delete;

    void prepare(const AudioConfig& config, const Session& initial_session);
    void reset() noexcept;

    // UI/control thread. Returns false when the bounded mailbox is full.
    [[nodiscard]] bool submit_session(const Session& session) noexcept;

    // Audio thread. No allocation, file access, locks, or exceptions.
    void process(std::span<StereoFrame> output) noexcept;

    // UI thread. Atomically snapshots meters produced by the audio callback.
    [[nodiscard]] AudioTelemetry telemetry() const noexcept;

    // UI thread. Clears voices and effect memory on the next audio block.
    void panic() noexcept;

    [[nodiscard]] float sample_rate() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace cursed_drone
