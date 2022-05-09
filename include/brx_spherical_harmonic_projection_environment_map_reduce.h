//
// Copyright (C) YuqiaoZhang(HanetakaChou)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef _BRX_SPHERICAL_HARMONIC_PROJECTION_ENVIRONMENT_MAP_REDUCE_H_
#define _BRX_SPHERICAL_HARMONIC_PROJECTION_ENVIRONMENT_MAP_REDUCE_H_ 1

#include "brx_spherical_harmonic_projection_environment_map.h"

static inline DirectX::XMUINT3 brx_sh_projection_environment_map_dispatch_extent(DirectX::XMUINT2 environment_map_size)
{
    uint32_t const tile_num_width = static_cast<uint32_t>(environment_map_size.x + static_cast<uint32_t>(BRX_SH_PROJECTION_ENVIRONMENT_MAP_THREAD_GROUP_X) - 1U) / static_cast<uint32_t>(BRX_SH_PROJECTION_ENVIRONMENT_MAP_THREAD_GROUP_X);
    uint32_t const tile_num_height = static_cast<uint32_t>(environment_map_size.y + static_cast<uint32_t>(BRX_SH_PROJECTION_ENVIRONMENT_MAP_THREAD_GROUP_Y) - 1U) / static_cast<uint32_t>(BRX_SH_PROJECTION_ENVIRONMENT_MAP_THREAD_GROUP_Y);

    static_assert(1U == static_cast<uint32_t>(BRX_SH_PROJECTION_ENVIRONMENT_MAP_THREAD_GROUP_Z), "");

    return DirectX::XMUINT3(
        tile_num_width,
        tile_num_height,
        1U);
}

#endif
