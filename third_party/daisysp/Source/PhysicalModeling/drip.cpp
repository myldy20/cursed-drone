/*
Copyright (c) 2020 Electrosmith, Corp, Paul Batchelor

MIT-licensed DaisySP Drip, vendored from commit
599511b740f8f3a9b8db72a0642aa45b8a23c3a3. The two resonator assignments
marked below correct an apparent upstream port typo (member instead of local
filter input); without it, two of the three water resonances remain silent.
*/
#include "drip.h"

#include "dsp.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace daisysp {
namespace {
constexpr float kSoundDecay = 0.95F;
constexpr float kSystemDecay = 0.996F;
constexpr float kResonance = 0.9985F;
constexpr float kFrequencySweep = 1.0001F;
constexpr float kMaximumShake = 2000.0F;
}

int Drip::my_random(int maximum) { return std::rand() % (maximum + 1); }

float Drip::noise_tick() {
    const float value = static_cast<float>(std::rand()) - 1'073'741'823.5F;
    return value * (1.0F / 1'073'741'823.0F);
}

void Drip::Init(float sample_rate, float dettack) {
    sample_rate_ = sample_rate;
    dettack_ = dettack;
    num_tubes_ = 10.0F;
    damp_ = 0.2F;
    shake_max_ = 0.0F;
    freq_ = 450.0F;
    freq1_ = 600.0F;
    freq2_ = 720.0F;
    amp_ = 0.3F;
    snd_level_ = 0.0F;
    const float radians = 2.0F * PI_F / sample_rate_;
    kloop_ = sample_rate_ * dettack_;
    outputs00_ = outputs01_ = outputs10_ = outputs11_ = outputs20_ = outputs21_ = 0.0F;
    center_freqs0_ = res_freq0_ = 450.0F;
    center_freqs1_ = res_freq1_ = 600.0F;
    center_freqs2_ = res_freq2_ = 750.0F;
    num_objects_ = 10.0F;
    sound_decay_ = kSoundDecay;
    system_decay_ = kSystemDecay;
    const float gain = std::log(10.0F) / 10.0F;
    gains0_ = gains1_ = gains2_ = gain;
    coeffs01_ = coeffs11_ = coeffs21_ = kResonance * kResonance;
    coeffs00_ = -kResonance * 2.0F * std::cos(450.0F * radians);
    coeffs10_ = -kResonance * 2.0F * std::cos(600.0F * radians);
    coeffs20_ = -kResonance * 2.0F * std::cos(750.0F * radians);
    shake_energy_ = amp_ * kMaximumShake * 0.1F;
    shake_damp_ = 0.0F;
    shake_max_save_ = 0.0F;
    final_z0_ = final_z1_ = final_z2_ = 0.0F;
}

void Drip::SetFrequency(float frequency) {
    freq_ = std::max(80.0F, frequency * 0.78F);
    freq1_ = std::max(100.0F, frequency);
    freq2_ = std::max(120.0F, frequency * 1.27F);
}

void Drip::SetDamping(float damping) { damp_ = std::clamp(damping, 0.0F, 1.0F); }

void Drip::SetDensity(float density) { num_tubes_ = 1.0F + std::clamp(density, 0.0F, 1.0F) * 24.0F; }

float Drip::Process(bool trigger) {
    if (trigger) Init(sample_rate_, dettack_);
    const float radians = 2.0F * PI_F / sample_rate_;
    if (num_tubes_ != 0.0F && num_tubes_ != num_objects_) {
        num_objects_ = std::max(1.0F, num_tubes_);
    }
    if (freq_ != 0.0F && freq_ != res_freq0_) {
        res_freq0_ = freq_;
        coeffs00_ = -kResonance * 2.0F * std::cos(res_freq0_ * radians);
    }
    if (damp_ != 0.0F && damp_ != shake_damp_) {
        shake_damp_ = damp_;
        system_decay_ = kSystemDecay + shake_damp_ * 0.002F;
    }
    if (shake_max_ != 0.0F && shake_max_ != shake_max_save_) {
        shake_max_save_ = shake_max_;
        shake_energy_ = std::min(kMaximumShake,
            shake_energy_ + shake_max_save_ * kMaximumShake * 0.1F);
    }
    if (freq1_ != 0.0F && freq1_ != res_freq1_) {
        res_freq1_ = freq1_;
        coeffs10_ = -kResonance * 2.0F * std::cos(res_freq1_ * radians);
    }
    if (freq2_ != 0.0F && freq2_ != res_freq2_) {
        res_freq2_ = freq2_;
        coeffs20_ = -kResonance * 2.0F * std::cos(res_freq2_ * radians);
    }
    if (--kloop_ == 0.0F) shake_energy_ = 0.0F;

    shake_energy_ *= system_decay_;
    snd_level_ = shake_energy_ * sound_decay_;
    if (my_random(32767) < static_cast<int>(num_objects_)) {
        const int selected = my_random(3);
        if (selected == 0) {
            center_freqs0_ = res_freq1_ * (0.75F + 0.25F * noise_tick());
            gains0_ = std::abs(noise_tick());
        } else if (selected == 1) {
            center_freqs1_ = res_freq1_ * (1.0F + 0.25F * noise_tick());
            gains1_ = std::abs(noise_tick());
        } else {
            center_freqs2_ = res_freq1_ * (1.25F + 0.25F * noise_tick());
            gains2_ = std::abs(noise_tick());
        }
    }

    gains0_ *= kResonance;
    gains1_ *= kResonance;
    gains2_ *= kResonance;
    if (gains0_ > 0.001F) {
        center_freqs0_ *= kFrequencySweep;
        coeffs00_ = -kResonance * 2.0F * std::cos(center_freqs0_ * radians);
    }
    if (gains1_ > 0.0F) {
        center_freqs1_ *= kFrequencySweep;
        coeffs10_ = -kResonance * 2.0F * std::cos(center_freqs1_ * radians);
    }
    if (gains2_ > 0.001F) {
        center_freqs2_ *= kFrequencySweep;
        coeffs20_ = -kResonance * 2.0F * std::cos(center_freqs2_ * radians);
    }

    float input0 = snd_level_ * noise_tick();
    float input1 = input0 * gains1_;
    float input2 = input0 * gains2_;
    input0 *= gains0_;
    input0 -= outputs00_ * coeffs00_ + outputs01_ * coeffs01_;
    outputs01_ = outputs00_;
    outputs00_ = input0;
    float data = gains0_ * outputs00_;
    input1 -= outputs10_ * coeffs10_ + outputs11_ * coeffs11_;
    outputs11_ = outputs10_;
    outputs10_ = input1; // corrected DaisySP port typo
    data += gains1_ * outputs10_;
    input2 -= outputs20_ * coeffs20_ + outputs21_ * coeffs21_;
    outputs21_ = outputs20_;
    outputs20_ = input2; // corrected DaisySP port typo
    data += gains2_ * outputs20_;

    final_z2_ = final_z1_;
    final_z1_ = final_z0_;
    final_z0_ = data * 4.0F;
    return (final_z2_ - final_z0_) * 0.005F;
}

} // namespace daisysp
