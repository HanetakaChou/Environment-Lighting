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

void main(
	in float3 in_position : POSITION0, 
	in float3 in_normal : NORMAL0, 
	out float4 d3d_Position : SV_POSITION, 
	out float3 out_position : TEXCOORD0, 
	out float3 out_normal : TEXCOORD1
	)
{
	float3 model_position = in_position;
	float3 world_position = mul(model_transform, float4(model_position, 1.0)).xyz;
	float4 clip_position = mul(projection_transform, mul(view_transform, float4(world_position, 1.0)));

	float3 model_normal = in_normal;
	// TODO: normal transform
	float3x3 tangent_transform = float3x3(model_transform[0].xyz, model_transform[1].xyz, model_transform[2].xyz);
	float3 world_normal = normalize(mul(tangent_transform, model_normal));

	out_position = world_position;
	out_normal = world_normal;
	d3d_Position = clip_position;
}