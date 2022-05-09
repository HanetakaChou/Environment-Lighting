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

#ifndef _BRDF_SAMPLING_HLSLI_
#define _BRDF_SAMPLING_HLSLI_ 1

#include "../thirdparty/Brioche-Shader-Language/shaders/brx_math_consts.bsli"

float3 tr_sample_omega_h(float alpha, float2 xi)
{
    // PBRT-V3: [TrowbridgeReitzDistribution::Sample_wh](https://github.com/mmp/pbrt-v3/blob/book/src/core/microfacet.cpp#L308)  
    // UE: [ImportanceSampleGGX](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/MonteCarlo.ush#L331)
    // U3D: [SampleGGXDir](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/ImageBasedLighting.hlsl#L127)  

    float tan_2_theta_h = alpha * alpha * xi.x / (1.0 - xi.x);
    float cos_theta_h = 1.0 / sqrt(1.0 + tan_2_theta_h);
    float sin_theta_h = sqrt(1.0 - cos_theta_h * cos_theta_h);
    float phi = 2.0 * BRX_M_PI * xi.y;

    float3 omega_h_tangent_space = float3(sin_theta_h * cos(phi), sin_theta_h * sin(phi), cos_theta_h);
    return omega_h_tangent_space;
}

#endif