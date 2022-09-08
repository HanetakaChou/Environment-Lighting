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
#include "integrate_ld_normal_distribution_pipeline_layout.h"

TextureCube Distant_Radiance_Distribution : register(t0);

SamplerState clamp_point_sampler : register(s0);

// https://docs.microsoft.com/en-us/windows/win32/direct3d11/direct3d-11-advanced-stages-compute-shader
// D3D11_CS_THREAD_GROUP_MAX_X 1024
// D3D11_CS_THREAD_GROUP_MAX_Y 1024
#define THREAD_GROUP_X 32
#define THREAD_GROUP_Y 32

#define SAMPLE_COUNT (THREAD_GROUP_X * THREAD_GROUP_Y)
#define LDS_COUNT (SAMPLE_COUNT / 2)

// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-variable-syntax
// "in D3D11 the maximum size is 32kb"
// 8192 = 4 * 3 * 512 + 4 * 512
groupshared float3 lds_sum_numerator[LDS_COUNT];
groupshared float lds_sum_denominator[LDS_COUNT];

#define MAX_CUBE_MIP_LEVEL_COUNT 8
#define MAX_CUBE_SIZE (1 << (MAX_CUBE_MIP_LEVEL_COUNT - 1))
#define CUBE_FACE_COUNT 6

// Direct3D 11 only supports 8 UAVs
RWTexture2DArray<float4> LD_Normal_Distribution[MAX_CUBE_MIP_LEVEL_COUNT] : register(u0);

[numthreads(THREAD_GROUP_X, THREAD_GROUP_Y, 1)]
void main(
	uint3 work_group_id : SV_GroupID,
	uint3 local_invocation_id : SV_GroupThreadID,
	uint local_invocation_index : SV_GroupIndex)
{
	// UE: [FilterPS](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShaders.usf#L330)
	// UE: [FilterCS](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShaders.usf#L319)
	// U3D: [RenderCubemapGGXConvolution](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.high-definition/Runtime/Sky/SkyManager.cs#L508)

	uint LD_Normal_Distribution_Width;
	uint LD_Normal_Distribution_Height;
	// uint LD_Normal_Distribution_MipLevelCount
	{
		uint out_width;
		uint out_height;
		uint out_elements;
		LD_Normal_Distribution[0].GetDimensions(out_width, out_height, out_elements);
		LD_Normal_Distribution_Width = out_width;
		LD_Normal_Distribution_Height = out_height;
	}

	const uint roughness_mip_level = work_group_id.z / CUBE_FACE_COUNT;
	const uint cube_face_index = work_group_id.z - CUBE_FACE_COUNT * roughness_mip_level;
	const uint cube_index_u = work_group_id.x;
	const uint cube_index_v = work_group_id.y;

	const uint cube_size_u = LD_Normal_Distribution_Width >> roughness_mip_level;
	const uint cube_size_v = LD_Normal_Distribution_Height >> roughness_mip_level;

	[branch]
	if (MAX_CUBE_SIZE < cube_size_u || MAX_CUBE_SIZE < cube_size_v || CUBE_FACE_COUNT <= cube_face_index || MAX_CUBE_MIP_LEVEL_COUNT <= roughness_mip_level)
	{
		[branch]
		if (cube_index_u < cube_size_u && cube_index_v < cube_size_v && cube_face_index < CUBE_FACE_COUNT && roughness_mip_level < MAX_CUBE_MIP_LEVEL_COUNT)
		{
			[branch]
			if (0 == local_invocation_index)
			{
				// To help debug
				float4 error_color = float4(7777.77, 7777.77, 7777.77, 7777.77);
				[branch]
				if (0 == roughness_mip_level)
				{
					LD_Normal_Distribution[0][uint3(cube_index_u, cube_index_v, cube_face_index)] = error_color;
				}
				else if (1 == roughness_mip_level)
				{
					LD_Normal_Distribution[1][uint3(cube_index_u, cube_index_v, cube_face_index)] = error_color;
				}
				else if (2 == roughness_mip_level)
				{
					LD_Normal_Distribution[2][uint3(cube_index_u, cube_index_v, cube_face_index)] = error_color;
				}
				else if (3 == roughness_mip_level)
				{
					LD_Normal_Distribution[3][uint3(cube_index_u, cube_index_v, cube_face_index)] = error_color;
				}
				else if (4 == roughness_mip_level)
				{
					LD_Normal_Distribution[4][uint3(cube_index_u, cube_index_v, cube_face_index)] = error_color;
				}
				else if (5 == roughness_mip_level)
				{
					LD_Normal_Distribution[5][uint3(cube_index_u, cube_index_v, cube_face_index)] = error_color;
				}
				else if (6 == roughness_mip_level)
				{
					LD_Normal_Distribution[6][uint3(cube_index_u, cube_index_v, cube_face_index)] = error_color;
				}
				else if (7 == roughness_mip_level)
				{
					LD_Normal_Distribution[7][uint3(cube_index_u, cube_index_v, cube_face_index)] = error_color;
				}
				else
				{
					// Error!
					// 8 == MAX_CUBE_MIP_LEVEL_COUNT
					// TODO: error color to help debug
				}
			}
		}

		return;
	}

	[branch]
	if (!(cube_index_u < cube_size_u && cube_index_v < cube_size_v))
	{
		return;
	}

	// Ideally, we have three dimensions: N (world space), NdotV and roughness.
	// roughness is mapped to mip_level.
	// [Karis 2013]: assume "V=N" ⇒ V=N=R and NdotV=1 ⇒ both N and NdotV are mapped to R.
	// TODO: If we use the Paraboloid Map, perhaps we can use the 3D texture and provide 1 dimension for the NdotV. Thus, we can use both NdotV and N to look up the LD term.  
	float roughness;
	{
		// UE: [ComputeReflectionCaptureRoughnessFromMip](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShared.ush#L30)
		// U3D: [MipmapLevelToPerceptualRoughness](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/ImageBasedLighting.hlsl#L61)

		const float REFLECTION_CAPTURE_ROUGHEST_MIP = 1.0;
		const float REFLECTION_CAPTURE_ROUGHNESS_MIP_SCALE = 1.2;
		float LevelFrom1x1 = (float(LD_Normal_Distribution_MipLevelCount) - 1.0) - 1.0 - float(roughness_mip_level);
		roughness = exp2((REFLECTION_CAPTURE_ROUGHEST_MIP - LevelFrom1x1) / REFLECTION_CAPTURE_ROUGHNESS_MIP_SCALE);
	}

	// Real-Time Rendering Fourth Edition / 9.8.1 Normal Distribution Functions: "In the Disney principled shading model, Burley[214] exposes the roughness control to users as αg = r2, where r is the user-interface roughness parameter value between 0 and 1."
	float alpha = roughness * roughness;

	float3 cube_map_direction;
	float d_omega_p;
	{
		float u = ((float(cube_index_u) + 0.5) / float(cube_size_u)) * 2.0 - 1.0;
		float v = ((float(cube_index_v) + 0.5) / float(cube_size_v)) * 2.0 - 1.0;

		// The coordinates between RWTexture2DArray and TextureCube should be consistent.
		// Mapping from uint3(cube_index_u, cube_index_v, cube_face_index) to float3(cube_map_direction).
		// UE4: [GetCubemapVector] (https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShaders.usf#L89)
		float3 cube_map_direction_all_faces[CUBE_FACE_COUNT] = {
			// X+
			float3(1.0, -v, -u),
			// X-
			float3(-1.0, -v, u),
			// Y+
			float3(u, 1.0, v),
			// Y-
			float3(u, -1.0, -v),
			// Z+
			float3(u, -v, 1.0),
			// Z-
			float3(-u, -v, -1.0)
		};
		cube_map_direction = cube_map_direction_all_faces[cube_face_index];

		// "Projection from Cube Maps" of [Sloan 2008]: "fWt = 4/(sqrt(fTmp)*fTmp)"
		// UE4: [DiffuseIrradianceCopyPS](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShaders.usf#L448) 
		// DirectXMath: [SHProjectCubeMap](https://github.com/microsoft/DirectXMath/blob/jul2018b/SHMath/DirectXSHD3D11.cpp#L341) 
		float d_a = (1.0 - (-1.0)) * (1.0 - (-1.0)) / LD_Normal_Distribution_Width / LD_Normal_Distribution_Height;
		float r_2 = 1.0 * 1.0 + u * u + v * v;
		float cos_theta = rsqrt(r_2);
		d_omega_p = d_a * cos_theta / r_2;
	}

	// [Karis 2013]: we assume "V = N" when pre-integrating the LD term, while the reflection direction V is used to look up the LD term when rendering.
	float3 omega_n_world_space = normalize(cube_map_direction);

	// Since the TR BRDF is isotropic, the outgoing direction V is **usually** assumed to be in the XOZ plane.  
	// But since we assume "V = N", the TR BDRF becomes radially symmetric and the tangent direction is arbitrary.
	// UE: [GetTangentBasis](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/MonteCarlo.ush#L12)
	// U3D: [GetLocalFrame](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/CommonLighting.hlsl#L408)
	float3x3 tangent_to_world_transform;
	{
		// "TangentZ" should be normalized.
		const float3 TangentZ = omega_n_world_space;

		const float Sign = TangentZ.z >= 0 ? 1 : -1;
		const float a = -rcp(Sign + TangentZ.z);
		const float b = TangentZ.x * TangentZ.y * a;

		float3 TangentX = { 1 + Sign * a * TangentZ.x * TangentZ.x, Sign * b, -Sign * TangentZ.x };
		float3 TangentY = { b,  Sign + a * TangentZ.y * TangentZ.y, -TangentZ.y };

		tangent_to_world_transform = float3x3(
			float3(TangentX.x, TangentY.x, TangentZ.x),  // row 0
			float3(TangentX.y, TangentY.y, TangentZ.y),  // row 1
			float3(TangentX.z, TangentY.z, TangentZ.z)   // row 2
			);
	}

	// We only sum one sample for the local value.
	float3 local_sum_numerator = float3(0.0, 0.0, 0.0);
	float local_sum_denominator = 0.0;
	{
		uint sample_index = local_invocation_index;

		float2 xi = hammersley_2d(sample_index, SAMPLE_COUNT);

		float3 omega_h_tangent_space = tr_sample_omega_h(alpha, xi);

		// V = N ⇒ V = (0, 0, 1) 
		// L = 2.0 * dot(V, H) * H - V = 2.0 * H.z * H - float3(0, 0, 1)
		float3 omega_l_tangent_space = 2.0 * omega_h_tangent_space.z * omega_h_tangent_space - float3(0, 0, 1);
		
		// Tangent Space
		// N = (0, 0, 1) 
		float non_clamped_NdotH = omega_h_tangent_space.z;
		float non_clamped_NdotL = omega_l_tangent_space.z;

		[branch]
		if (non_clamped_NdotH > 0.0 && non_clamped_NdotL > 0.0)
		{
			float NdotH = saturate(non_clamped_NdotH);
			float NdotL = saturate(non_clamped_NdotL);

			// LD = (∫ L * BRDF * cos_theta_L dL) / (∫ BRDF * max(0, cos_theta_L) dL).
			// BRDF * max(0, cos_theta_L)  = (F * D * G * max(0, NdotL)) / (4 * max(0, NdotL) * max(0, NdotV)) = (F * D * G) / (4 * max(0, NdotV)).
			// PDF = D * max(0, NdotH) / (4 * max(0, LdotH)).
			// Weight = (BRDF * cos_theta_L) / PDF = (F * G * max(0, LdotH)) / (max(0, NdotV) * max(0, NdotH)).

			// [Karis 2013] assumes that (V == N) and thus (LdotH == NdotH) && (NdotV == 1).  
			// PDF = D / 4.
			// Weight = F * G.
			float PDF = D_TR(alpha, NdotH) * (1.0 / 4.0);

			// [Lagarde 2014] proves that when L is constant, the result does NOT depend on the Weight.
			// [Karis 2013] assumes that Weight = max(0, NdotL).
			float weight = NdotL;

			// "20.4 Mipmap Filtered Samples" of GPU Gems 3
			float sample_mip_level;
			{
				// UE: [SolidAngleTexel](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShaders.usf#L355)
				// U3D: [invOmegaP](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/ImageBasedLighting.hlsl#L506)
				// float d_omega_p = 4.0 * PI / 6.0 / cube_size_u / cube_size_v;

				// UE: [SolidAngleSample](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShaders.usf#L414)
				// U3D: [omegaS](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/ImageBasedLighting.hlsl#L500)
				float d_omega_s = (1.0 / float(SAMPLE_COUNT)) / PDF;

				// UE: [Mip](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShaders.usf#L415)
				// U3D: [mipLevel](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/ImageBasedLighting.hlsl#L506)
				sample_mip_level = max(0.0, 0.5 * log2(d_omega_s / d_omega_p) - 0.5);
			}

			float3 omega_l_world_space = mul(tangent_to_world_transform, omega_l_tangent_space);

			float3 L_omega_l = Distant_Radiance_Distribution.SampleLevel(clamp_point_sampler, omega_l_world_space, sample_mip_level).rgb;

			local_sum_numerator += L_omega_l * weight;
			local_sum_denominator += weight;

		}
	}

	// Parallel Reduction
	float3 ld_normal_distribution = float3(0.777, 0.777, 0.777);
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
			float3 total_sum_numerator = lds_sum_numerator[lds_index] + lds_sum_numerator[lds_index + 1];
			float total_sum_denominator = lds_sum_denominator[lds_index] + lds_sum_denominator[lds_index + 1];

			ld_normal_distribution = total_sum_numerator / total_sum_denominator;
		}
	}

	// Output to global memory
	[branch]
	if (0 == local_invocation_index)
	{
		[branch]
		if (0 == roughness_mip_level)
		{
			LD_Normal_Distribution[0][uint3(cube_index_u, cube_index_v, cube_face_index)] = float4(ld_normal_distribution, 1.0);
		}
		else if (1 == roughness_mip_level)
		{
			LD_Normal_Distribution[1][uint3(cube_index_u, cube_index_v, cube_face_index)] = float4(ld_normal_distribution, 1.0);
		}
		else if (2 == roughness_mip_level)
		{
			LD_Normal_Distribution[2][uint3(cube_index_u, cube_index_v, cube_face_index)] = float4(ld_normal_distribution, 1.0);
		}
		else if (3 == roughness_mip_level)
		{
			LD_Normal_Distribution[3][uint3(cube_index_u, cube_index_v, cube_face_index)] = float4(ld_normal_distribution, 1.0);
		}
		else if (4 == roughness_mip_level)
		{
			LD_Normal_Distribution[4][uint3(cube_index_u, cube_index_v, cube_face_index)] = float4(ld_normal_distribution, 1.0);
		}
		else if (5 == roughness_mip_level)
		{
			LD_Normal_Distribution[5][uint3(cube_index_u, cube_index_v, cube_face_index)] = float4(ld_normal_distribution, 1.0);
		}
		else if (6 == roughness_mip_level)
		{
			LD_Normal_Distribution[6][uint3(cube_index_u, cube_index_v, cube_face_index)] = float4(ld_normal_distribution, 1.0);
		}
		else if (7 == roughness_mip_level)
		{
			LD_Normal_Distribution[7][uint3(cube_index_u, cube_index_v, cube_face_index)] = float4(ld_normal_distribution, 1.0);
		}
		else
		{
			// Error!
			// 8 == MAX_CUBE_MIP_LEVEL_COUNT
			// TODO: error color to help debug
		}
	}
}

