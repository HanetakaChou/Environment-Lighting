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

#include "low_discrepancy_sequence.hlsli"
#include "brdf_sampling.hlsli"
#include "BRDF.hlsli"

// https://docs.microsoft.com/en-us/windows/win32/direct3d11/direct3d-11-advanced-stages-compute-shader
// D3D11_CS_THREAD_GROUP_MAX_X 1024
// D3D11_CS_THREAD_GROUP_MAX_Y 1024
#define THREAD_GROUP_X 32
#define THREAD_GROUP_Y 32

#define SAMPLE_COUNT (THREAD_GROUP_X * THREAD_GROUP_Y)
#define LDS_COUNT (SAMPLE_COUNT / 2)

// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-variable-syntax
// "in D3D11 the maximum size is 32kb"
// 6144 = 4 * 2 * 512 + 4 * 512
groupshared float2 lds_sum_numerator[LDS_COUNT];
groupshared float lds_sum_denominator[LDS_COUNT];

#define Hemispherical_Directional_Reflectance_LUT_INDEX_TR 0

RWTexture2DArray<float4> Hemispherical_Directional_Reflectance_LUT : register(u0);

[numthreads(THREAD_GROUP_X, THREAD_GROUP_Y, 1)]
void main(
	uint3 work_group_id : SV_GroupID,
	uint3 local_invocation_id : SV_GroupThreadID,
	uint local_invocation_index : SV_GroupIndex)
{
	uint Hemispherical_Directional_Reflectance_LUT_Width;
	uint Hemispherical_Directional_Reflectance_LUT_Height;
	uint Hemispherical_Directional_Reflectance_LUT_Array_Layer_Count;
	{
		uint out_width;
		uint out_height;
		uint out_elements;
		Hemispherical_Directional_Reflectance_LUT.GetDimensions(out_width, out_height, out_elements);
		Hemispherical_Directional_Reflectance_LUT_Width = out_width;
		Hemispherical_Directional_Reflectance_LUT_Height = out_height;
		Hemispherical_Directional_Reflectance_LUT_Array_Layer_Count = out_elements;
	}

	const uint lut_index_u = work_group_id.x;
	const uint lut_index_v = work_group_id.y;
	const uint lut_index_brdf = work_group_id.z;

	[branch]
	if (!(lut_index_u < Hemispherical_Directional_Reflectance_LUT_Width && lut_index_v < Hemispherical_Directional_Reflectance_LUT_Height && lut_index_brdf < Hemispherical_Directional_Reflectance_LUT_Array_Layer_Count))
	{
		return;
	}

	// The coordinates between RWTexture2D and Texture should be consistent.
	// TODO: Should it be flipped in OpenGL?
	const float2 lut_uv = float2((float(lut_index_u) + 0.5) / float(Hemispherical_Directional_Reflectance_LUT_Width), (float(lut_index_v) + 0.5) / float(Hemispherical_Directional_Reflectance_LUT_Height));

	const float NdotV = lut_uv.x;
	const float roughness = lut_uv.y;

	// Since the TR BRDF is isotropic, the outgoing direction V is assumed to be in the XOZ plane.  
	const float3 omega_v_tangent_space = float3(sqrt(1.0 * 1.0 - NdotV * NdotV), 0.0, NdotV);

	// Real-Time Rendering Fourth Edition / 9.8.1 Normal Distribution Functions: "In the Disney principled shading model, Burley[214] exposes the roughness control to users as ��g = r2, where r is the user-interface roughness parameter value between 0 and 1."
	float alpha = roughness * roughness;

	// We only sum one sample for the local value.
	float2 local_sum_numerator = float2(0.0, 0.0);
	float local_sum_denominator = 0.0;

	// No divergence
	[branch]
	if(Hemispherical_Directional_Reflectance_LUT_INDEX_TR == lut_index_brdf)
	{
		uint sample_index = local_invocation_index;

		float2 xi = hammersley_2d(sample_index, SAMPLE_COUNT);

		float3 omega_h_tangent_space = tr_sample_omega_h(alpha, xi);

		// L = 2.0 * dot(V, H) * H - V
		float3 omega_l_tangent_space = 2.0 * dot(omega_v_tangent_space, omega_h_tangent_space) * omega_h_tangent_space - omega_v_tangent_space;

		// Tangent Space
		// N = (0, 0, 1) 
		float non_clamped_NdotH = omega_h_tangent_space.z;
		float non_clamped_NdotL = omega_l_tangent_space.z;

		float non_clamped_VdotH = dot(omega_v_tangent_space, omega_h_tangent_space);

		[branch]
		if (non_clamped_NdotH > 0.0 && non_clamped_NdotL > 0.0 && non_clamped_VdotH > 0.0)
		{
			float NdotH = saturate(non_clamped_NdotH);
			float NdotL = saturate(non_clamped_NdotL);
			float VdotH = saturate(non_clamped_VdotH);

			// n_R = (∫ (1 - (1 - VdotH)^5) * D * V * max(0, cos_theta_L) dL).
			// n_G = (∫ (1 - VdotH)^5 * D * V * max(0, cos_theta_L) dL).
            // D * V * max(0, cos_theta_L) = D * V * max(0, NdotL)).
            // PDF = D * max(0, NdotH) / (4 * max(0, VdotH)).
            // term_common = (D * V * max(0, cos_theta_L)) / PDF = (V * max(0, NdotL) * 4 * max(0, VdotH)) / (max(0, NdotH)).
			float term_common = V_HC_TR(alpha, NdotV, NdotL) * NdotL * 4.0 * VdotH / NdotH;
			float x = 1.0 - VdotH;
			float x2 = x * x;
			float x5 = x2 * x2 * x;
			float n_R = (1.0 - x5) * term_common;
			float n_G = x5 * term_common;

			local_sum_numerator += float2(n_R, n_G);
		}

		local_sum_denominator += 1.0;
	}

	// Parallel Reduction
	float2 hemispherical_directional_reflectance = float2(0.777, 0.777);
	{
		uint linear_index = local_invocation_index;

		// Half of the LDS can be saved by the following method:
		// Half threads store the local values into the LDS, and the other threads read back these values from the LDS and reduce them with their local values.
		GroupMemoryBarrierWithGroupSync();

		[branch]
		if (linear_index >= LDS_COUNT && linear_index < (LDS_COUNT * 2))
		{
			uint lds_index = linear_index - LDS_COUNT;
			lds_sum_numerator[lds_index] = local_sum_numerator;
			lds_sum_denominator[lds_index] = local_sum_denominator;
		}

		GroupMemoryBarrierWithGroupSync();

		[branch]
		if (linear_index < LDS_COUNT)
		{
			uint lds_index = linear_index;
			lds_sum_numerator[lds_index] = local_sum_numerator + lds_sum_numerator[lds_index];
			lds_sum_denominator[lds_index] = local_sum_denominator + lds_sum_denominator[lds_index];
		}

#if 0
		[unroll]
		for (uint k = (LDS_COUNT / 2); k > 1; k /= 2)
		{
			GroupMemoryBarrierWithGroupSync();

			[branch]
			if (linear_index < k)
			{
				uint lds_index = linear_index;
				lds_sum_numerator[lds_index] = lds_sum_numerator[lds_index] + lds_sum_numerator[lds_index + k];
				lds_sum_denominator[lds_index] = lds_sum_denominator[lds_index] + lds_sum_denominator[lds_index + k];
			}
		}
#else
		[unroll]
		for (uint k = firstbithigh(LDS_COUNT / 2); k > 0; --k)
		{
			GroupMemoryBarrierWithGroupSync();

			[branch]
			if (linear_index < (1 << k))
			{
				uint lds_index = linear_index;
				lds_sum_numerator[lds_index] = lds_sum_numerator[lds_index] + lds_sum_numerator[lds_index + (1 << k)];
				lds_sum_denominator[lds_index] = lds_sum_denominator[lds_index] + lds_sum_denominator[lds_index + (1 << k)];
			}
		}
#endif

		GroupMemoryBarrierWithGroupSync();

		[branch]
		if (0 == linear_index)
		{
			uint lds_index = linear_index;
			float2 total_sum_numerator = lds_sum_numerator[lds_index] + lds_sum_numerator[lds_index + 1];
			float total_sum_denominator = lds_sum_denominator[lds_index] + lds_sum_denominator[lds_index + 1];

			hemispherical_directional_reflectance = total_sum_numerator / total_sum_denominator;
		}
	}


	// Output to global memory
	[branch]
	if (0 == local_invocation_index)
	{
		Hemispherical_Directional_Reflectance_LUT[uint3(lut_index_u, lut_index_v, lut_index_brdf)] = float4(hemispherical_directional_reflectance, 0.0, 1.0);
	}
}