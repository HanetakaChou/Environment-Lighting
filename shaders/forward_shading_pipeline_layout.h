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

#ifndef _FORWARD_SHADING_PIPELINE_LAYOUT_H_
#define _FORWARD_SHADING_PIPELINE_LAYOUT_H_ 1

#if defined(__STDC__) || defined(__cplusplus)

struct forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer
{
	DirectX::XMFLOAT4X4 view_transform;
	DirectX::XMFLOAT4X4 projection_transform;
	DirectX::XMFLOAT3 eye_position;
	float __padding_align16_eye_position;
};

struct forward_shading_pipeline_layout_global_set_object_binding_uniform_buffer
{
	DirectX::XMFLOAT4X4 model_transform;
};


#elif defined(HLSL_VERSION) || defined(__HLSL_VERSION)

cbuffer forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer : register(b0)
{
	column_major float4x4 view_transform;
	column_major float4x4 projection_transform;
	float3 eye_position;
	float __padding_align16_eye_position;

};

cbuffer forward_shading_pipeline_layout_global_set_object_binding_uniform_buffer : register(b1)
{
	column_major float4x4 model_transform;
};

ByteAddressBuffer t_environment_map_sh_coefficients : register(t0);

Texture2D t_lut_specular_hdr_fresnel_factors : register(t1);

Texture2DArray t_lut_specular_transfer_function_sh_coefficients : register(t2);

SamplerState s_clamp_linear_sampler : register(s0);

#else
#error Unknown Compiler
#endif

#endif