// SPDX-License-Identifier: GPL-3.0-or-later
#include "cursed_drone/i18n.hpp"

#include <array>
#include <cstddef>

namespace cursed_drone {
namespace {

using Catalog = std::array<std::string_view, 24>;

constexpr Catalog kRussian{
    "Проклятый гудёж",
    "Управляемые звуковые пространства",
    "ЯЗЫК",
    "СЛОТ",
    "ДВИЖОК",
    "ЭФФЕКТЫ",
    "МОДУЛЯТОРЫ",
    "МАСТЕР",
    "ЧАСТОТА",
    "ТЕМБР",
    "ЦВЕТ",
    "ДВИЖЕНИЕ",
    "ТЕКСТУРА",
    "УРОВЕНЬ",
    "ПАНОРАМА",
    "ЗАГЛУШЁН",
    "РАБОТАЕТ",
    "Сессия сохранена",
    "Сессия загружена",
    "Рендер готов",
    "Управление",
    "</> слот / ^/V параметр / A/D значение / Tab экран / F фейд / K kill",
    "Диагностический тон проверяет тракт и не является продуктовым синтезатором.",
    "Ошибка",
};

constexpr Catalog kEnglish{
    "Cursed Drone",
    "Controllable sound spaces",
    "LANGUAGE",
    "SLOT",
    "ENGINE",
    "EFFECTS",
    "MODULATORS",
    "MASTER",
    "FREQUENCY",
    "TIMBRE",
    "COLOR",
    "MOTION",
    "TEXTURE",
    "LEVEL",
    "PAN",
    "MUTED",
    "RUNNING",
    "Session saved",
    "Session loaded",
    "Render complete",
    "Controls",
    "</> slot / ^/V parameter / A/D value / Tab page / F fade / K kill",
    "The diagnostic tone validates the signal path and is not a product synthesizer.",
    "Error",
};

} // namespace

std::string_view tr(Locale locale, TextId id) {
    const auto index = static_cast<std::size_t>(id);
    return locale == Locale::ru ? kRussian.at(index) : kEnglish.at(index);
}

} // namespace cursed_drone
