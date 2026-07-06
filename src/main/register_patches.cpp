#include "ovl_patches.hpp"
#include "../../RecompiledPatches/recomp_overlays.inl"

#include "librecomp/overlays.hpp"
#include "librecomp/game.hpp"

extern "C" {
    extern const char game_patches_bin[];
    extern const size_t game_patches_bin_size;
}

void harvestmoon64::register_patches() {
    recomp::overlays::register_patches(game_patches_bin, game_patches_bin_size, section_table, ARRLEN(section_table));
    recomp::overlays::register_base_exports(export_table);
    recomp::overlays::register_base_events(event_names);
    recomp::overlays::register_manual_patch_symbols(manual_patch_symbols);
}
