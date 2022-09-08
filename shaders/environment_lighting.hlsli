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
#include "math_consts.hlsli"
#include "BRDF.hlsli"

float3 EvaluateDiffuseEnvironmentLighting(float3 diffuse_color, float3 specular_color, float3 N, float4 form_factor_normal_distribution[7])
{
    // UE: [DiffuseIrradianceCopyPS](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShaders.usf#L438)
    // UE: [ComputeSkyEnvMapDiffuseIrradianceCS](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ReflectionEnvironmentShaders.usf#L534)
    // U3D: [AmbientProbeConvolution](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.high-definition/Runtime/Sky/AmbientProbeConvolution.compute#L36)

    // UE: [GetSkyLighting](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BasePassPixelShader.usf#L557)
    // U3D: [ProbeVolumeEvaluateSphericalHarmonics](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.high-definition/Runtime/Lighting/LightLoop/LightLoop.hlsl#L553)

    float3 irradiance_normal;
    {
        // "Stupid Spherical Harmonics (SH)" / "Appendix A10 Shader/CPU code for Irradiance Environment Maps"

        // P(0, 0) =  0.282094791773878140
        // P(1,-1) = -0.488602511902919920*y
        // P(1, 0) =  0.488602511902919920*z
        // P(1, 1) = -0.488602511902919920*x
        // P(2,-2) =  1.092548430592079200*x*y
        // P(2,-1) = -1.092548430592079200*y*z
        // P(2, 0) =  0.946174695757560080*z*z + -0.315391565252520050
        // P(2, 1) = -1.092548430592079200*x*z
        // P(2, 2) =  0.546274215296039590*(x*x - y*y)

        // x: P(1, 1)
        // y: P(1,-1)
        // z: P(1, 0)
        // w: P(0, 0) + part of P(2, 0)
        float4 cAr = form_factor_normal_distribution[0];
        float4 cAg = form_factor_normal_distribution[1];
        float4 cAb = form_factor_normal_distribution[2];
        float3 x1 = float3(dot(cAr, float4(N, 1.0)), dot(cAg, float4(N, 1.0)), dot(cAb, float4(N, 1.0)));

        // x: P(2,-2)
        // y: P(2,-1)
        // z: part of P(2, 0)
        // w: P(2, 1)
        float4 cBr = form_factor_normal_distribution[3];
        float4 cBg = form_factor_normal_distribution[4];
        float4 cBb = form_factor_normal_distribution[5];
        float4 vB = N.xyzz * N.yzzx;
        float3 x2 = float3(dot(cBr, vB), dot(cBg, vB), dot(cBb, vB));

        // r: P(2, 2)
        // g: P(2, 2)
        // b: P(2, 2)
        float vC = N.x * N.x - N.y * N.y;
        float4 cC = form_factor_normal_distribution[6];
        float3 x3 = cC.rgb * vC;

        float3 form_factor_normal = x1 + x2 + x3;

        irradiance_normal = PI * form_factor_normal;
    }

    // UE: [EnvBRDFApproxFullyRough](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BasePassPixelShader.usf#L1055)
    // U3D: [GetDiffuseOrDefaultColor](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.high-definition/Runtime/Material/Lit/Lit.hlsl#L1237)
    {
        diffuse_color = diffuse_color + specular_color * 0.45;
    }

    float3 diffuse_radiance = Diffuse_Lambert(diffuse_color) * irradiance_normal;

    return diffuse_radiance;
}

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

    float4 form_factor_normal_distribution[7];
    {
        form_factor_normal_distribution[0] = asfloat(Form_Factor_Normal_Distribution.Load4(0));
        form_factor_normal_distribution[1] = asfloat(Form_Factor_Normal_Distribution.Load4(4 * 4 * 1));
        form_factor_normal_distribution[2] = asfloat(Form_Factor_Normal_Distribution.Load4(4 * 4 * 2));
        form_factor_normal_distribution[3] = asfloat(Form_Factor_Normal_Distribution.Load4(4 * 4 * 3));
        form_factor_normal_distribution[4] = asfloat(Form_Factor_Normal_Distribution.Load4(4 * 4 * 4));
        form_factor_normal_distribution[5] = asfloat(Form_Factor_Normal_Distribution.Load4(4 * 4 * 5));
        form_factor_normal_distribution[6] = asfloat(Form_Factor_Normal_Distribution.Load4(4 * 4 * 6));
    }
    float3 diffuse_radiance = EvaluateDiffuseEnvironmentLighting(diffuse_color, specular_color, N, form_factor_normal_distribution);

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