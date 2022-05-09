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


Texture2D g_environment_map : register(t0);

SamplerState g_environment_map_sampler : register(s0);

RWByteAddressBuffer g_irradiance_coefficients : register(u0);

#define BRX_SH_PROJECTION_ENVIRONMENT_MAP_REDUCE_LAYOUT BRX_SH_PROJECTION_ENVIRONMENT_MAP_REDUCE_LAYOUT_OCTAHEDRAL_MAP
#define INTERNAL_BRX_DISABLE_ROOT_SIGNATURE
#include "../thirdparty/Spherical-Harmonic/shaders/brx_spherical_harmonic_projection_environment_map_reduce.bsli"

brx_int2 brx_sh_application_bridge_get_environment_map_dimension()
{
    return brx_texture_2d_get_dimension(g_environment_map, 0);
}

brx_float3 brx_sh_application_bridge_get_environment_map_lighting(in brx_int2 texture_coordinates)
{
    return brx_fetch_2d(g_environment_map, brx_int3(texture_coordinates, 0)).rgb;
}

brx_uint brx_sh_application_bridge_get_environment_map_sh_coefficient(in brx_int in_environment_map_sh_coefficient_monochromatic_index)
{
    return brx_byte_address_buffer_load(g_irradiance_coefficients, (in_environment_map_sh_coefficient_monochromatic_index << 2));
}

brx_uint brx_sh_application_bridge_compare_and_swap_environment_map_sh_coefficient(in brx_int in_environment_map_sh_coefficient_monochromatic_index, in brx_uint in_old_value, in brx_uint in_new_value)
{
    return brx_byte_address_buffer_interlocked_compare_exchange(g_irradiance_coefficients, (in_environment_map_sh_coefficient_monochromatic_index << 2), in_old_value, in_new_value);
}
