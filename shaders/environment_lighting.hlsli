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
#include "hemispherical_hirectional_reflectance.hlsli"
#include "../thirdparty/Brioche-Shader-Language/shaders/brx_math_consts.bsli"
#include "../thirdparty/Environment-Lighting/shaders/brx_Image_based_lighting.bsli"

float3 EvaluateSpecularEnvironmentLighting(float3 specular_color, float roughness, float3 N, float3 R, float NdotV, TextureCube ld_normal_distribution)
{
    // UE: [ReflectionEnvironment](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentPixelShader.usf#L205)
    // U3D: [EvaluateBSDF_Env](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.high-definition/Runtime/Lighting/LightLoop/LightLoop.hlsl#L454)

    float3 DFG_Term = Hemispherical_Directional_Reflectance_TR(specular_color, roughness, NdotV);

    float3 LD_Term;
    {
        // UE: [GetSkyLightReflection](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShared.ush#L38)
        // U3D: [SampleEnvWithDistanceBaseRoughness](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.high-definition/Runtime/Lighting/LightEvaluation.hlsl#L600)

        // Ideally, we have three dimensions: N (world space), NdotV and roughness.
        // roughness is mapped to mip_level.
        // [Karis 2013]: assume "V=N" ⇒ V=N=R and NdotV=1 ⇒ both N and NdotV are mapped to R.
        // TODO: If we use the Paraboloid Map, perhaps we can use the 3D texture and provide 1 dimension for the NdotV. Thus, we can use both NdotV and N to look up the LD term.

        float3 biased_R;
        {
            // Real-Time Rendering Fourth Edition / 9.8.1 Normal Distribution Functions: "In the Disney principled shading model, Burley[214] exposes the roughness control to users as αg = r2, where r is the user-interface roughness parameter value between 0 and 1."
            float alpha = roughness * roughness;

            // UE: [GetOffSpecularPeakReflectionDir](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShared.ush#L120)
            // U3D: [GetSpecularDominantDir](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/ImageBasedLighting.hlsl#L105)
            biased_R = lerp(N, R, (sqrt(1.0 - alpha) + alpha) * (1.0 - alpha));
        }

        float roughness_mip_level;
        {
            // UE: [ComputeReflectionCaptureMipFromRoughness](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShared.ush#L21)
            // U3D: [PerceptualRoughnessToMipmapLevel](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/ImageBasedLighting.hlsl#L44)

            float out_width;
            float out_height;
            float out_number_of_levels;
            ld_normal_distribution.GetDimensions(0, out_width, out_height, out_number_of_levels);

            int CubemapMaxMip = out_number_of_levels - 1;

            const float REFLECTION_CAPTURE_ROUGHEST_MIP = 1.0;
            const float REFLECTION_CAPTURE_ROUGHNESS_MIP_SCALE = 1.2;
            float LevelFrom1x1 = REFLECTION_CAPTURE_ROUGHEST_MIP - REFLECTION_CAPTURE_ROUGHNESS_MIP_SCALE * log2(max(roughness, 0.001));

            float AbsoluteSpecularMip = float(CubemapMaxMip) - 1.0 - LevelFrom1x1;

            roughness_mip_level = max(0.0, AbsoluteSpecularMip);
        }

        LD_Term = ld_normal_distribution.SampleLevel(clamp_linear_sampler, R, roughness_mip_level).rgb;
    }

    float3 specular_radiance = DFG_Term * LD_Term;

    return specular_radiance;
}

float3 EvaluateEnvironmentLighting(float3 diffuse_color, float3 specular_color, float roughness, float3 N, float3 V, float raw_ambient_occlusion)
{
    //  float3 R = reflect(-V, N);
    float3 R = 2.0 * dot(V, N) * N - V;
    // UE: [DefaultLitBxDF]https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ShadingModels.ush#L218
    // U3D: [ClampNdotV](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/CommonLighting.hlsl#L349)
    float NdotV = clamp(dot(N, V), 0.0, 1.0); // 0.0001, 1.0);

    brx_float3 irradiance_coefficients[9];
    for (brx_int irradiance_coefficient_index = 0; irradiance_coefficient_index < 9; ++irradiance_coefficient_index)
    {
        irradiance_coefficients[irradiance_coefficient_index] = brx_uint_as_float(brx_byte_address_buffer_load3(Form_Factor_Normal_Distribution, (4 * 3) * irradiance_coefficient_index));
    }

    float3 diffuse_radiance = brx_image_based_diffuse_lighting(diffuse_color, specular_color, N, irradiance_coefficients);

    float3 specular_radiance = EvaluateSpecularEnvironmentLighting(specular_color, roughness, N, R, NdotV, LD_Normal_Distribution);

    float3 ambient_occlusion = AO_InterReflections(AO_BentNormal(raw_ambient_occlusion, N), diffuse_color);

    // UE: [GBufferAO](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BasePassPixelShader.usf#L1072)
    float specular_color_luminance = dot(specular_color, float3(0.3, 0.59, 0.11));
    float raw_directional_occlusion = AO_InterReflections(DO_BentNormal(raw_ambient_occlusion, N), float3(specular_color_luminance, specular_color_luminance, specular_color_luminance)).g;

    float directional_occlusion = DO_FromAO(raw_directional_occlusion, roughness, NdotV);

    // U3D: [ApplyAmbientOcclusionFactor](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.high-definition/Runtime/Material/Lit/Lit.hlsl#L2020)

    return (diffuse_radiance * ambient_occlusion + specular_radiance * directional_occlusion);
}

#endif