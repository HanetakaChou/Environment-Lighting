#version 310 es

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

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_samplerless_texture_functions : enable
#extension GL_EXT_control_flow_attributes : enable

#include "unified_integrate_pipeline_layout.h"
#include "sh_projecting.glsli"
#include "math_consts.glsli"

struct Packed_SH_L2_RGB
{
	// x: P(1, 1)
	// y: P(1,-1)
	// z: P(1, 0)
	// w: P(0, 0) + part of P(2, 0)
	highp vec4 cAr;
	highp vec4 cAg;
	highp vec4 cAb;

	// x: P(2,-2)
	// y: P(2,-1)
	// z: part of P(2, 0)
	// w: P(2, 1)
	highp vec4 cBr;
	highp vec4 cBg;
	highp vec4 cBb;

	// r: P(2, 2)
	// g: P(2, 2)
	// b: P(2, 2)
	highp vec3 cC;
};

Packed_SH_L2_RGB evaluate_sh_l2_rgb_clamped_cosine(SH_L2_RGB fLight);

// https://docs.microsoft.com/en-us/windows/win32/direct3d11/direct3d-11-advanced-stages-compute-shader
// D3D11_CS_THREAD_GROUP_MAX_X 1024
// D3D11_CS_THREAD_GROUP_MAX_Y 1024
#define THREAD_GROUP_X 16
#define THREAD_GROUP_Y 16

#define LDS_COUNT (THREAD_GROUP_X * THREAD_GROUP_Y / 2)

// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-variable-syntax
// "in D3D11 the maximum size is 32kb"
// 14336 = 4 * 9 * 3 * 128 + 4 * 128
shared SH_L2_RGB lds_sum_numerator[LDS_COUNT];
shared float lds_sum_denominator[LDS_COUNT];

#define MAX_CUBE_SIZE 4096
#define CUBE_FACE_COUNT 6

// Avoid infinite loop
#define MAX_THREAD_GROUP_X (MAX_CUBE_SIZE / THREAD_GROUP_X)
#define MAX_THREAD_GROUP_Y (MAX_CUBE_SIZE / THREAD_GROUP_Y)

layout(local_size_x = THREAD_GROUP_X, local_size_y = THREAD_GROUP_Y, local_size_z = 1) in;

void main()
{
	highp ivec3 work_group_id = ivec3(gl_WorkGroupID.x, gl_WorkGroupID.y, gl_WorkGroupID.z);
	highp ivec3 local_invocation_id = ivec3(gl_LocalInvocationID.x, gl_LocalInvocationID.y, gl_LocalInvocationID.z);
	highp int local_invocation_index = int(gl_LocalInvocationIndex);

	// TODO: We may use two level reduction rather than the loop?
	[[dont_flatten]]
	if (0 != work_group_id.x || 0 != work_group_id.y || 0 != work_group_id.z)
	{
		return;
	}

	// UE: [DiffuseIrradianceCopyPS](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShaders.usf#L438)
	// UE: [ComputeSkyEnvMapDiffuseIrradianceCS](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShaders.usf#L534)
	// U3D: [AmbientProbeConvolution](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.high-definition/Runtime/Sky/AmbientProbeConvolution.compute#L36)
	// DirectXMath: [SHProjectCubeMap](https://github.com/microsoft/DirectXMath/blob/jul2018b/SHMath/DirectXSHD3D11.cpp#L169)

	highp int cube_size_u;
	highp int cube_size_v;
	{
	    highp ivec2 out_width_height = textureSize(Distant_Radiance_Distribution, 0);
		cube_size_u = out_width_height.x;
		cube_size_v = out_width_height.y;
	}

	[[dont_flatten]]
	if(MAX_CUBE_SIZE < cube_size_u || MAX_CUBE_SIZE < cube_size_v)
	{
		[[dont_flatten]]
		if (0 == local_invocation_index)
		{
			// To help debug
			highp vec4 error_color = vec4(7777.77, 7777.77, 7777.77, 7777.77);		
			Form_Factor_Normal_Distribution.cAr = error_color;
			Form_Factor_Normal_Distribution.cAg = error_color;
			Form_Factor_Normal_Distribution.cAb = error_color;
			Form_Factor_Normal_Distribution.cBr = error_color;
			Form_Factor_Normal_Distribution.cBg = error_color;
			Form_Factor_Normal_Distribution.cBb = error_color;
			Form_Factor_Normal_Distribution.cC = error_color.xyz;
		}

		return;
	}

	const highp int tile_size_u = THREAD_GROUP_X;
	const highp int tile_size_v = THREAD_GROUP_Y;

	// Each thread calculates (tile_num_u * tile_num_v * CUBE_FACE_COUNT) samples.
	highp int tile_num_u = int(float(cube_size_u + tile_size_u - 1) / float(tile_size_u));
	highp int tile_num_v = int(float(cube_size_v + tile_size_v - 1) / float(tile_size_v));

	SH_L2_RGB local_sum_numerator = evaluate_sh_l2_rgb_zero();
	highp float local_sum_denominator = 0.0;
	// [[unroll]]
	for (highp int cube_face_index = 0; cube_face_index < CUBE_FACE_COUNT; ++cube_face_index)
	{
		[[dont_unroll]]
		for (highp int tile_index_u = 0; tile_index_u < MAX_THREAD_GROUP_X && tile_index_u < tile_num_u; ++tile_index_u)
		{
			[[dont_unroll]]
			for (highp int tile_index_v = 0; tile_index_v < MAX_THREAD_GROUP_Y && tile_index_v < tile_num_v; ++tile_index_v)
			{
				highp int cube_index_u = tile_size_u * tile_index_u + local_invocation_id.x;
				highp int cube_index_v = tile_size_v * tile_index_v + local_invocation_id.y;

				[[dont_flatten]]
				if (cube_index_u < cube_size_u && cube_index_v < cube_size_v)
				{
					highp vec3 cube_map_direction;
					highp float d_omega_mul_cube_size;
					{
						float u = ((float(cube_index_u) + 0.5) / float(cube_size_u)) * 2.0 - 1.0;
						float v = ((float(cube_index_v) + 0.5) / float(cube_size_v)) * 2.0 - 1.0;

						// The coordinates between RWTexture2DArray and TextureCube should be consistent.
						// Mapping from uint3(cube_index_u, cube_index_v, cube_face_index) to vec3(cube_map_direction).
						// UE4: [GetCubemapVector] (https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShaders.usf#L89)
						highp vec3 cube_map_direction_all_faces[CUBE_FACE_COUNT] = vec3[CUBE_FACE_COUNT](
							// X+
							vec3(1.0, -v, -u),
							// X-
							vec3(-1.0, -v, u),
							// Y+
							vec3(u, 1.0, v),
							// Y-
							vec3(u, -1.0, -v),
							// Z+
							vec3(u, -v, 1.0),
							// Z-
							vec3(-u, -v, -1.0)
						);
						cube_map_direction = cube_map_direction_all_faces[cube_face_index];

						// "Projection from Cube Maps" of [Sloan 2008]: "fWt = 4/(sqrt(fTmp)*fTmp)"
						// The common divisor "1/(cube_size_u*cube_size_v)" is reduced.
						// UE4: [DiffuseIrradianceCopyPS](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShaders.usf#L448) 
						// DirectXMath: [SHProjectCubeMap](https://github.com/microsoft/DirectXMath/blob/jul2018b/SHMath/DirectXSHD3D11.cpp#L341) 
						highp float d_a_mul_cube_size = (1.0 - (-1.0)) * (1.0 - (-1.0)); // / cube_size_u / cube_size_v
						highp float r_2 = 1.0 * 1.0 + u * u + v * v;
						highp float cos_theta = inversesqrt(r_2);
						d_omega_mul_cube_size = d_a_mul_cube_size * cos_theta / r_2;
					}

					highp vec3 omega = normalize(cube_map_direction);

					highp vec3 L_omega = textureLod(samplerCube(Distant_Radiance_Distribution, clamp_point_sampler), omega, 0.0).rgb;

					// NOTE: omega should be normalized before using the "polynomial form"
					SH_L2 upsilon = evaluate_sh_l2_basis(omega);

					SH_L2_RGB product = evaluate_sh_l2_scale_rgb(upsilon, L_omega);

					SH_L2_RGB sample_numerator = evaluate_sh_l2_rgb_scale(product, d_omega_mul_cube_size);

					// accumulate
					local_sum_numerator = evaluate_sh_l2_rgb_add(local_sum_numerator, sample_numerator);

					// expected to be "(4*PI)*(cube_size_u*cube_size_v)"
					local_sum_denominator += d_omega_mul_cube_size;
				}
			}
		}
	}

	// Parallel Reduction
	SH_L2_RGB distant_radiance_distribution_sh_projection = evaluate_sh_l2_rgb_zero();
	{
		highp int linear_index = local_invocation_index;

		// Half of the LDS can be saved by the following method:
		// Half threads store the local values into the LDS, and the other threads read back these values from the LDS and reduce them with their local values.
		memoryBarrierShared();
		barrier();

		[[dont_flatten]]
		if (linear_index >= LDS_COUNT && linear_index < (LDS_COUNT * 2))
		{
			highp int lds_index = linear_index - LDS_COUNT;
			lds_sum_numerator[lds_index] = local_sum_numerator;
			lds_sum_denominator[lds_index] = local_sum_denominator;
		}

		memoryBarrierShared();
		barrier();

		[[dont_flatten]]
		if (linear_index < LDS_COUNT)
		{
			highp int lds_index = linear_index;
			lds_sum_numerator[lds_index] = evaluate_sh_l2_rgb_add(local_sum_numerator, lds_sum_numerator[lds_index]);
			lds_sum_denominator[lds_index] = local_sum_denominator + lds_sum_denominator[lds_index];
		}

		[[unroll]]
		for (highp int k = (LDS_COUNT / 2); k > 1; k /= 2)
		{
			memoryBarrierShared();
			barrier();

			[[dont_flatten]]
			if (linear_index < k)
			{
				highp int lds_index = linear_index;
				lds_sum_numerator[lds_index] = evaluate_sh_l2_rgb_add(lds_sum_numerator[lds_index], lds_sum_numerator[lds_index + k]);
				lds_sum_denominator[lds_index] = lds_sum_denominator[lds_index] + lds_sum_denominator[lds_index + k];
			}
		}

		memoryBarrierShared();
		barrier();

		[[dont_flatten]]
		if (0 == linear_index)
		{
			highp int lds_index = linear_index;
			SH_L2_RGB total_sum_numerator = evaluate_sh_l2_rgb_add(lds_sum_numerator[lds_index], lds_sum_numerator[lds_index + 1]);
			highp float total_sum_denominator = lds_sum_denominator[lds_index] + lds_sum_denominator[lds_index + 1];

			distant_radiance_distribution_sh_projection = evaluate_sh_l2_rgb_scale(total_sum_numerator, (4.0 * PI) / total_sum_denominator);
		}
	}

	// Output to global memory
	[[dont_flatten]]
	if (0 == local_invocation_index)
	{
		Packed_SH_L2_RGB form_factor_normal_distribution = evaluate_sh_l2_rgb_clamped_cosine(distant_radiance_distribution_sh_projection);
		Form_Factor_Normal_Distribution.cAr = form_factor_normal_distribution.cAr;
		Form_Factor_Normal_Distribution.cAg = form_factor_normal_distribution.cAg;
		Form_Factor_Normal_Distribution.cAb = form_factor_normal_distribution.cAb;
		Form_Factor_Normal_Distribution.cBr = form_factor_normal_distribution.cBr;
		Form_Factor_Normal_Distribution.cBg = form_factor_normal_distribution.cBg;
		Form_Factor_Normal_Distribution.cBb = form_factor_normal_distribution.cBb;
		Form_Factor_Normal_Distribution.cC = form_factor_normal_distribution.cC;
	}

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//    IMPLEMENTATION
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Packed_SH_L2_RGB evaluate_sh_l2_rgb_clamped_cosine(SH_L2_RGB fLight)
{
	// UE4: [SetupSkyIrradianceEnvironmentMapConstantsFromSkyIrradiance](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Source/Runtime/Renderer/Private/SceneRendering.cpp#L979)
	// UE4: [ComputeSkyEnvMapDiffuseIrradianceCS](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShaders.usf#L607)
	// U3D: [PackCoefficients](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.high-definition/Runtime/Lighting/SphericalHarmonics.cs#L196)

	// "Appendix A10" of [Sloan 2008]: "SetSHEMapConstants"
	// F = (1 / PI) * E = (1 / PI) * (sqrt((4 * PI) / (2l + 1)) * SH(L_omega) * ZH(cos_theta)) = ((1 / PI) * sqrt((4 * PI) / (2l + 1)) * ZH(cos_theta)) * SH(L_omega)
	// "(1 / PI) * sqrt((4 * PI) / (2l + 1)) * ZH(cos_theta)" is precalculated.
	// Since the "PI" is divided, it is the "form factor" rather than the "irradiance" that [Sloan 2008] calculates.
	const float fC0 = 0.282094791773878140;
	const float fC1 = 0.325735007935279930;
	const float fC2 = 0.273137107648019790;
	const float fC3 = 0.078847891313130011;
	const float fC4 = 0.136568553824009900;

	Packed_SH_L2_RGB vCoeff;

	// x: P(1, 1)
	// y: P(1,-1)
	// z: P(1, 0)
	// w: P(0, 0) + part of P(2, 0)
	vCoeff.cAr.x = -fC1 * fLight.r.p[3];
	vCoeff.cAr.y = -fC1 * fLight.r.p[1];
	vCoeff.cAr.z = fC1 * fLight.r.p[2];
	vCoeff.cAr.w = fC0 * fLight.r.p[0] - fC3 * fLight.r.p[6];

	vCoeff.cAg.x = -fC1 * fLight.g.p[3];
	vCoeff.cAg.y = -fC1 * fLight.g.p[1];
	vCoeff.cAg.z = fC1 * fLight.g.p[2];
	vCoeff.cAg.w = fC0 * fLight.g.p[0] - fC3 * fLight.g.p[6];

	vCoeff.cAb.x = -fC1 * fLight.b.p[3];
	vCoeff.cAb.y = -fC1 * fLight.b.p[1];
	vCoeff.cAb.z = fC1 * fLight.b.p[2];
	vCoeff.cAb.w = fC0 * fLight.b.p[0] - fC3 * fLight.b.p[6];

	// x: P(2,-2)
	// y: P(2,-1)
	// z: part of P(2, 0)
	// w: P(2, 1)
	vCoeff.cBr.x = fC2 * fLight.r.p[4];
	vCoeff.cBr.y = -fC2 * fLight.r.p[5];
	vCoeff.cBr.z = 3.0 * fC3 * fLight.r.p[6];
	vCoeff.cBr.w = -fC2 * fLight.r.p[7];

	vCoeff.cBg.x = fC2 * fLight.g.p[4];
	vCoeff.cBg.y = -fC2 * fLight.g.p[5];
	vCoeff.cBg.z = 3.0 * fC3 * fLight.g.p[6];
	vCoeff.cBg.w = -fC2 * fLight.g.p[7];

	vCoeff.cBb.x = fC2 * fLight.b.p[4];
	vCoeff.cBb.y = -fC2 * fLight.b.p[5];
	vCoeff.cBb.z = 3.0 * fC3 * fLight.b.p[6];
	vCoeff.cBb.w = -fC2 * fLight.b.p[7];

	// r: P(2, 2)
	// g: P(2, 2)
	// b: P(2, 2)
	vCoeff.cC.r = fC4 * fLight.r.p[8];
	vCoeff.cC.g = fC4 * fLight.g.p[8];
	vCoeff.cC.b = fC4 * fLight.b.p[8];

	return vCoeff;
}