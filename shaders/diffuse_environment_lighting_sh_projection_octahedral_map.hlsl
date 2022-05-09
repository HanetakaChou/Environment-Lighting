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

#define BRX_DIFFUSE_ENVIRONMENT_LIGHTING_SH_PROJECTION_REDUCE_LAYOUT BRX_DIFFUSE_ENVIRONMENT_LIGHTING_SH_PROJECTION_REDUCE_LAYOUT_OCTAHEDRAL_MAP
#define INTERNAL_DISABLE_ROOT_SIGNATURE
#include "../thirdparty/Environment-Lighting/shaders/brx_diffuse_environment_lighting_sh_projection_reduce.bsli"
