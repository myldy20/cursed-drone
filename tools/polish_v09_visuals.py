#!/usr/bin/env python3
from pathlib import Path

path = Path('src/sdl_main.cpp')
text = path.read_text(encoding='utf-8')
old = '''        case cd::EffectKind::comb: {
            const float teeth = 2.0F + (1.0F - effect.tone) * 12.0F;
            const float resonance = std::pow(std::abs(std::sin(t * 3.14159265F * teeth)),
                3.0F + effect.feedback * 9.0F);
            value = ((1.0F - effect.amount) + resonance * effect.amount) * 2.0F - 1.0F;
            break;
        }
        case cd::EffectKind::bypass:
        case cd::EffectKind::delay:
            break;'''
new = '''        case cd::EffectKind::comb: {
            const float teeth = 2.0F + (1.0F - effect.tone) * 12.0F;
            const float resonance = std::pow(std::abs(std::sin(t * 3.14159265F * teeth)),
                3.0F + effect.feedback * 9.0F);
            value = ((1.0F - effect.amount) + resonance * effect.amount) * 2.0F - 1.0F;
            break;
        }
        case cd::EffectKind::chorus:
        case cd::EffectKind::flanger:
        case cd::EffectKind::phaser:
            value = std::sin(t * 6.2831853F * (1.0F + effect.tone * 7.0F) +
                std::sin(t * 12.5663706F) * effect.feedback * 2.2F) * effect.amount;
            break;
        case cd::EffectKind::diffuser:
            value = (std::sin(t * 18.8495559F) + std::sin(t * 43.9822972F) * 0.52F +
                std::sin(t * 69.1150384F) * 0.25F) * effect.amount * 0.48F;
            break;
        case cd::EffectKind::ahdr: {
            const float phase = std::fmod(t * (1.0F + effect.tone * 3.0F), 1.0F);
            float envelope = 0.0F;
            if (phase < 0.13F) envelope = phase / 0.13F;
            else if (phase < 0.30F) envelope = 1.0F;
            else if (phase < 0.72F) envelope = 1.0F - (phase - 0.30F) * 1.55F;
            else envelope = std::max(0.0F, 0.35F * (1.0F - (phase - 0.72F) / 0.28F));
            value = envelope * effect.amount * 1.8F - 0.82F;
            break;
        }
        case cd::EffectKind::tape_void:
        case cd::EffectKind::black_hole:
        case cd::EffectKind::ritual_gate:
        case cd::EffectKind::rust_cloud:
        case cd::EffectKind::deep_sea:
            value = std::tanh((std::sin(t * 6.2831853F * (1.0F + effect.tone * 5.0F)) +
                std::sin(t * 18.8495559F * (1.0F + effect.feedback * 2.0F)) * 0.62F +
                std::sin(t * 50.2654825F) * effect.feedback * 0.24F) *
                (0.55F + effect.amount * 2.4F));
            break;
        case cd::EffectKind::bypass:
        case cd::EffectKind::delay:
            break;'''
count = text.count(old)
if count != 1:
    raise SystemExit(f'expected one visual switch block, found {count}')
path.write_text(text.replace(old, new), encoding='utf-8')
