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

#ifndef _BRX_SH_PROJECT_ENVIRONMENT_REDUCTION_H_
#define _BRX_SH_PROJECT_ENVIRONMENT_REDUCTION_H_ 1

// https://docs.microsoft.com/en-us/windows/win32/direct3d11/direct3d-11-advanced-stages-compute-shader
// D3D11_CS_THREAD_GROUP_MAX_X 1024
// D3D11_CS_THREAD_GROUP_MAX_Y 1024
// D3D11_CS_THREAD_GROUP_MAX_Z 64
// D3D11_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP 1024
#define BRX_SH_PROJECT_ENVIRONMENT_MAP_REDUCTION_THREAD_GROUP_X 16
#define BRX_SH_PROJECT_ENVIRONMENT_MAP_REDUCTION_THREAD_GROUP_Y 16
#define BRX_SH_PROJECT_ENVIRONMENT_MAP_REDUCTION_THREAD_GROUP_Z 1

#define BRX_SH_PROJECT_ENVIRONMENT_MAP_LAYOUT_UNDEFINED 0
#define BRX_SH_PROJECT_ENVIRONMENT_MAP_LAYOUT_EQUIRECTANGULAR 1
#define BRX_SH_PROJECT_ENVIRONMENT_MAP_LAYOUT_OCTAHEDRAL 2

// #define BRX_SH_PROJECT_ENVIRONMENT_MAP_LAYOUT BRX_SH_PROJECT_ENVIRONMENT_MAP_LAYOUT_EQUIRECTANGULAR
// #define BRX_SH_PROJECT_ENVIRONMENT_MAP_LAYOUT BRX_SH_PROJECT_ENVIRONMENT_MAP_LAYOUT_OCTAHEDRAL

// Resource Binding
// brx_texture_2d g_environment_map
// brx_sampler_state g_environment_map_sampler
// brx_write_only_byte_address_buffer g_irradiance_coefficients

// Dispatch
// tile_num_width = uint(float(environment_map_width + BRX_SH_PROJECT_ENVIRONMENT_MAP_REDUCTION_THREAD_GROUP_X - 1) / float(BRX_SH_PROJECT_ENVIRONMENT_MAP_REDUCTION_THREAD_GROUP_X))
// tile_num_height = uint(float(environment_map_height + BRX_SH_PROJECT_ENVIRONMENT_MAP_REDUCTION_THREAD_GROUP_Y - 1) / float(BRX_SH_PROJECT_ENVIRONMENT_MAP_REDUCTION_THREAD_GROUP_Y))
// dispatch(tile_num_width, tile_num_height, 1)

#endif
