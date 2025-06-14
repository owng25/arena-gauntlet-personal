#include "base_data_loader.h"
#include "data/hyper_config.h"
#include "utility/json_helper.h"

namespace simulation
{
bool BaseDataLoader::LoadHyperConfig(const nlohmann::json& json_object, HyperConfig* out_hyper_config) const
{
    if (!GetFixedPointIntValue(json_object, "SubHyperScalePercentage", &out_hyper_config->sub_hyper_scale_percentage))
    {
        return false;
    }

    if (!json_helper_.GetIntValue(json_object, "EnemiesRangeUnits", &out_hyper_config->enemies_range_units))
    {
        return false;
    }

    return true;
}

}  // namespace simulation
