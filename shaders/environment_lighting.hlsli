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

#ifndef _ENVIRONMENT_LIGHTING_HLSLI_
#define _ENVIRONMENT_LIGHTING_HLSLI_ 1

#include "ao.hlsli"
#include "../thirdparty/Brioche-Shader-Language/shaders/brx_shader_language.bsli"
#include "../thirdparty/Brioche-Shader-Language/shaders/brx_brdf.bsli"
#include "../thirdparty/Spherical-Harmonic/shaders/brx_spherical_harmonic_diffuse_radiance.bsli"
#include "../thirdparty/Spherical-Harmonic/shaders/brx_spherical_harmonic_specular_radiance.bsli"

float3 EvaluateEnvironmentLighting(float3 diffuse_color, float3 specular_color, float roughness, float3 N, float3 V, float raw_ambient_occlusion)
{
    brx_float3 environment_map_sh_coefficients[BRX_SH_COEFFICIENT_COUNT];
    for (brx_int environment_map_sh_coefficient_index = 0; environment_map_sh_coefficient_index < BRX_SH_COEFFICIENT_COUNT; ++environment_map_sh_coefficient_index)
    {
        environment_map_sh_coefficients[environment_map_sh_coefficient_index] = brx_uint_as_float(brx_byte_address_buffer_load3(t_environment_map_sh_coefficients, (4 * 3) * environment_map_sh_coefficient_index));
    }

    const brx_float alpha = brx_max(brx_float(BRX_TROWBRIDGE_REITZ_ALPHA_MINIMUM), roughness * roughness);
    const brx_float NdotV = brx_max(brx_float(BRX_TROWBRIDGE_REITZ_NDOTV_MINIMUM), brx_dot(N, V));

    brx_float2 raw_lut_uv = brx_float2(brx_max(0.0, 1.0 - NdotV), brx_max(0.0, 1.0 - alpha));

    // Remap: [0, 1] -> [0.5/size, 1.0 - 0.5/size]
    // U3D: [Remap01ToHalfTexelCoord](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/Common.hlsl#L661)
    // UE4: [N/A](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/RectLight.ush#L450)
    brx_int2 hdr_norms_lut_dimension = brx_texture_2d_get_dimension(t_lut_specular_hdr_fresnel_factors, 0);
    brx_float2 hdr_norms_lut_uv = (brx_float2(0.5, 0.5) + brx_float2(hdr_norms_lut_dimension.x - 1, hdr_norms_lut_dimension.y - 1) * raw_lut_uv) / brx_float2(hdr_norms_lut_dimension.x, hdr_norms_lut_dimension.y);

    brx_float2 fresnel_factor = brx_sample_level_2d(t_lut_specular_hdr_fresnel_factors, s_clamp_linear_sampler, hdr_norms_lut_uv, 0.0).xy;
    brx_float f0_factor = fresnel_factor.x;
    brx_float f90_factor = fresnel_factor.y;

    // UE4: [EnvBRDF](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L476)
    brx_float3 f0 = specular_color;
    brx_float f90 = brx_clamp(50.0 * f0.g, 0.0, 1.0);

    // UE4: [EnvBRDF](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L471)
    // U3D: [GetPreIntegratedFGDGGXAndDisneyDiffuse](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.high-definition/Runtime/Material/PreIntegratedFGD/PreIntegratedFGD.hlsl#L8)
    brx_float3 specular_albedo = f0 * f0_factor + float3(f90, f90, f90) * f90_factor;

    brx_int3 sh_transfer_functions_lut_dimension = brx_texture_2d_array_get_dimension(t_lut_specular_transfer_function_sh_coefficients, 0);
    brx_float2 sh_transfer_functions_lut_uv = (brx_float2(0.5, 0.5) + brx_float2(sh_transfer_functions_lut_dimension.x - 1, sh_transfer_functions_lut_dimension.y - 1) * raw_lut_uv) / brx_float2(sh_transfer_functions_lut_dimension.x, sh_transfer_functions_lut_dimension.y);

    brx_float non_rotation_transfer_function_lut_sh_coefficients[BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT];
    brx_unroll for (brx_int transfer_function_lut_sh_coefficient_index = 0; transfer_function_lut_sh_coefficient_index < BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT; ++transfer_function_lut_sh_coefficient_index)
    {
        non_rotation_transfer_function_lut_sh_coefficients[transfer_function_lut_sh_coefficient_index] = brx_sample_level_2d_array(t_lut_specular_transfer_function_sh_coefficients, s_clamp_linear_sampler, brx_float3(sh_transfer_functions_lut_uv, brx_float(transfer_function_lut_sh_coefficient_index)), 0.0).x;
    }

    brx_float3 diffuse_albedo = diffuse_color;

    brx_float3 diffuse_radiance = brx_sh_diffuse_radiance(diffuse_albedo, N, environment_map_sh_coefficients);

    brx_float3 specular_radiance = brx_sh_specular_radiance(specular_albedo, V, N, non_rotation_transfer_function_lut_sh_coefficients, environment_map_sh_coefficients);

    brx_float3 ambient_occlusion = AO_InterReflections(AO_BentNormal(raw_ambient_occlusion, N), diffuse_color);

    // UE: [GBufferAO](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BasePassPixelShader.usf#L1072)
    brx_float specular_color_luminance = brx_dot(specular_color, brx_float3(0.3, 0.59, 0.11));
    brx_float raw_directional_occlusion = AO_InterReflections(DO_BentNormal(raw_ambient_occlusion, N), brx_float3(specular_color_luminance, specular_color_luminance, specular_color_luminance)).g;

    brx_float directional_occlusion = DO_FromAO(raw_directional_occlusion, roughness, NdotV);

    // U3D: [ApplyAmbientOcclusionFactor](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.high-definition/Runtime/Material/Lit/Lit.hlsl#L2020)

    return (diffuse_radiance * ambient_occlusion + specular_radiance * directional_occlusion);
}

#endif