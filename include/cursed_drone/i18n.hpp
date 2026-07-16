// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "cursed_drone/session.hpp"

#include <string_view>

namespace cursed_drone {

enum class TextId {
    app_name,
    app_subtitle,
    language,
    slot,
    engine,
    effects,
    modulators,
    master,
    frequency,
    timbre,
    color,
    motion,
    texture,
    level,
    pan,
    muted,
    running,
    saved,
    loaded,
    render_complete,
    controls,
    help_line,
    diagnostic_warning,
    error
};

[[nodiscard]] std::string_view tr(Locale locale, TextId id);

} // namespace cursed_drone
