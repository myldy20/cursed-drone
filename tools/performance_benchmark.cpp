#include "cursed_drone/audio.hpp"
#include "cursed_drone/session.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <string_view>
#include <vector>

namespace cd = cursed_drone;

struct Scenario {
    std::string_view name;
    cd::Session session;
};

cd::Session all_engine(cd::EngineKind engine) {
    auto session = cd::make_default_session();
    session.performance.texture = 0.55F;
    session.performance.pulse = 0.35F;
    session.performance.chaos = 0.28F;
    session.performance.space = 0.32F;
    session.performance.events = 0.48F;
    for (std::size_t i = 0; i < cd::kSlotCount; ++i) {
        auto& slot = session.slots[i];
        slot.enabled = true;
        slot.engine = engine;
        slot.level = 0.28F;
        slot.frequency_hz = 34.0F + static_cast<float>(i) * 11.0F;
        for (auto& effect : slot.effects) effect.kind = cd::EffectKind::bypass;
    }
    for (auto& effect : session.master_effects) effect.kind = cd::EffectKind::bypass;
    return session;
}

cd::Session worst_case() {
    auto session = all_engine(cd::EngineKind::plaits);
    session.performance.texture = 0.82F;
    session.performance.pulse = 0.68F;
    session.performance.chaos = 0.66F;
    session.performance.space = 0.90F;
    session.performance.events = 0.70F;
    for (auto& slot : session.slots) {
        slot.plaits_model = cd::PlaitsModel::modal;
        for (auto& effect : slot.effects) {
            effect = {cd::EffectKind::black_hole, 0.72F, 0.67F, 0.72F};
        }
    }
    for (auto& effect : session.master_effects) {
        effect = {cd::EffectKind::black_hole, 0.70F, 0.62F, 0.70F};
    }
    return session;
}

void run(const Scenario& scenario, float seconds) {
    constexpr float sample_rate = 48'000.0F;
    constexpr std::size_t block_size = 512;
    const std::size_t total_frames = static_cast<std::size_t>(sample_rate * seconds);
    cd::AudioGraph graph;
    graph.prepare({sample_rate, block_size}, scenario.session);
    std::vector<cd::StereoFrame> block(block_size);

    // Warm caches and internal tails.
    for (int i = 0; i < 64; ++i) graph.process({block.data(), block.size()});

    const auto started = std::chrono::steady_clock::now();
    std::chrono::nanoseconds longest{};
    std::size_t rendered = 0;
    while (rendered < total_frames) {
        const std::size_t count = std::min(block_size, total_frames - rendered);
        const auto callback_started = std::chrono::steady_clock::now();
        graph.process({block.data(), count});
        longest = std::max(longest, std::chrono::steady_clock::now() - callback_started);
        rendered += count;
    }
    const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    const double realtime = static_cast<double>(seconds) / elapsed;
    const double cpu = 100.0 / realtime;
    const double longest_ms = std::chrono::duration<double, std::milli>(longest).count();
    std::printf("%-22s %8.2fx realtime  %6.2f%% core  max block %6.3f ms\n",
        scenario.name.data(), realtime, cpu, longest_ms);
}

int main(int argc, char** argv) {
    float seconds = 8.0F;
    if (argc > 1) seconds = std::max(1.0F, std::strtof(argv[1], nullptr));

    auto muted = cd::make_default_session();
    for (auto& slot : muted.slots) slot.enabled = false;
    for (auto& effect : muted.master_effects) effect.kind = cd::EffectKind::bypass;

    const std::array scenarios{
        Scenario{"all actors muted", muted},
        Scenario{"default derelict", cd::make_default_session()},
        Scenario{"four sub drones", all_engine(cd::EngineKind::sub_drone)},
        Scenario{"four bowed metal", all_engine(cd::EngineKind::bowed_metal)},
        Scenario{"four plaits", all_engine(cd::EngineKind::plaits)},
        Scenario{"20 black holes", worst_case()},
    };
    for (const auto& scenario : scenarios) run(scenario, seconds);
}
