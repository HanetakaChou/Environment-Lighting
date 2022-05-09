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

#include "forward_shading_pipeline_layout.h"
#include "environment_lighting.hlsli"

void main(
	in float4 d3d_Position : SV_POSITION,
	in float3 in_position : TEXCOORD0,
	in float3 in_normal : TEXCOORD1,
	out float4 out_color : SV_TARGET0)
{
	float3 diffuse_color = float3(0.5, 0.5, 0.5);
	float3 specular_color = float3(0.25, 0.25, 0.25);
	float roughness = 0.15;
	float3 P = in_position;
	float3 N = in_normal;
	float3 V = normalize(eye_position - in_position);
	float raw_ambient_occlusion = 1.0;

	float3 color = EvaluateEnvironmentLighting(diffuse_color, specular_color, roughness, N, V, raw_ambient_occlusion);

	out_color = float4(color, 1.0);
}