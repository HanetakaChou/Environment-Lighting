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
#include "../thirdparty/Brioche-Shader-Language/shaders/brx_math_consts.bsli"
#include "../thirdparty/Environment-Lighting/shaders/brx_environment_lighting_diffuse_radiance.bsli"

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

    float3 diffuse_radiance = brx_environment_lighting_diffuse_radiance(diffuse_color, specular_color, N, irradiance_coefficients);

    float3 ambient_occlusion = AO_InterReflections(AO_BentNormal(raw_ambient_occlusion, N), diffuse_color);

    // UE: [GBufferAO](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BasePassPixelShader.usf#L1072)
    float specular_color_luminance = dot(specular_color, float3(0.3, 0.59, 0.11));
    float raw_directional_occlusion = AO_InterReflections(DO_BentNormal(raw_ambient_occlusion, N), float3(specular_color_luminance, specular_color_luminance, specular_color_luminance)).g;

    float directional_occlusion = DO_FromAO(raw_directional_occlusion, roughness, NdotV);

    // U3D: [ApplyAmbientOcclusionFactor](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.high-definition/Runtime/Material/Lit/Lit.hlsl#L2020)

    return (diffuse_radiance * ambient_occlusion);
}

#endif