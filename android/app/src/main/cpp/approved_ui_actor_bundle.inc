// Keep the approved Actor layout, but replace the Sound page and the final
// dispatcher with the current control surface. Renaming avoids duplicating the
// large Events/Modulation implementation.
#define x_actor_sound_exact x_actor_sound_exact_previous
#define a_actor a_actor_previous
#include "approved_ui_actor_exact.inc"
#undef a_actor
#undef x_actor_sound_exact
#include "approved_ui_actor_polished.inc"
