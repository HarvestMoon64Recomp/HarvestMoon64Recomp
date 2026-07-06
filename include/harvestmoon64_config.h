#ifndef __HARVESTMOON64_CONFIG_H__
#define __HARVESTMOON64_CONFIG_H__

#include <filesystem>
#include <string>

namespace harvestmoon64 {
    inline const std::u8string program_id = u8"HarvestMoon64Recompiled";
    inline const std::string program_name = "Harvest Moon 64: Recompiled";

    namespace configkeys {
        namespace general {
        }
    }

    // TODO: Move loading configs to the runtime once we have a way to allow per-project customization.
    void init_config();
};

#endif
