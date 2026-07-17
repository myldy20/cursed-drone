/*
Copyright (c) 2020 Electrosmith, Corp, Paul Batchelor

Use of this source code is governed by an MIT-style license that can be found
in the bundled DaisySP LICENSE file. Vendored from DaisySP commit
599511b740f8f3a9b8db72a0642aa45b8a23c3a3.
*/
#pragma once

namespace daisysp {

/** Perry Cook / Soundpipe physical model of dripping water. */
class Drip {
public:
    void Init(float sample_rate, float dettack);
    float Process(bool trigger);
    void SetFrequency(float frequency);
    void SetDamping(float damping);
    void SetDensity(float density);

private:
    int my_random(int maximum);
    float noise_tick();

    float gains0_{0.0F};
    float gains1_{0.0F};
    float gains2_{0.0F};
    float kloop_{0.0F};
    float dettack_{0.0F};
    float num_tubes_{0.0F};
    float damp_{0.0F};
    float shake_max_{0.0F};
    float freq_{0.0F};
    float freq1_{0.0F};
    float freq2_{0.0F};
    float amp_{0.0F};
    float snd_level_{0.0F};
    float outputs00_{0.0F};
    float outputs01_{0.0F};
    float outputs10_{0.0F};
    float outputs11_{0.0F};
    float outputs20_{0.0F};
    float outputs21_{0.0F};
    float center_freqs0_{0.0F};
    float center_freqs1_{0.0F};
    float center_freqs2_{0.0F};
    float sound_decay_{0.0F};
    float system_decay_{0.0F};
    float final_z0_{0.0F};
    float final_z1_{0.0F};
    float final_z2_{0.0F};
    float coeffs01_{0.0F};
    float coeffs00_{0.0F};
    float coeffs11_{0.0F};
    float coeffs10_{0.0F};
    float coeffs21_{0.0F};
    float coeffs20_{0.0F};
    float shake_energy_{0.0F};
    float shake_damp_{0.0F};
    float shake_max_save_{0.0F};
    float num_objects_{0.0F};
    float sample_rate_{48'000.0F};
    float res_freq0_{0.0F};
    float res_freq1_{0.0F};
    float res_freq2_{0.0F};
};

} // namespace daisysp
