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

TextureCube g_environment_cube_map : register(t0);

SamplerState g_environment_cubemap_sampler : register(s0);

RWTexture2D<float4> g_environment_octahedral_map : register(u0);

#include "../thirdparty/Environment-Lighting/shaders/brx_octahedral_mapping.bsli"

[numthreads(1, 1, 1)]
void main(
	uint3 brx_group_id : SV_GroupID
	)
{
	brx_uint2 texture_size = brx_write_only_texture_2d_get_dimension(g_environment_octahedral_map);
	brx_uint texture_width = texture_size.x;
	brx_uint texture_height = texture_size.y;
	
	brx_float2 uv = (brx_float2(brx_group_id.xy) + brx_float2(0.5, 0.5)) / brx_float2(texture_width, texture_height);
	
	brx_float2 ndc = uv * brx_float2(2.0, -2.0) + brx_float2(-1.0, 1.0);

	brx_float3 omega = brx_octahedral_unmap(ndc);

	brx_float4 L_omega = g_environment_cube_map.SampleLevel(g_environment_cubemap_sampler, omega, 0);
	
	brx_store_2d(g_environment_octahedral_map, brx_int2(brx_group_id.xy), L_omega);
}