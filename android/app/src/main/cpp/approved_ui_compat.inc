constexpr int kSourceLandscape = 90;
constexpr int kSourceMusical = 91;
constexpr int kActionPanic = 94;
bool g_panic_requested = false;

void restore_landscape_actor(cd::Session& session, int actor) {
    cd::Session recipe = session;
    cd::apply_scene_recipe(recipe, session.scene);
    session.slots[static_cast<std::size_t>(actor)] =
        recipe.slots[static_cast<std::size_t>(actor)];
    session.scene_modified = true;
}
