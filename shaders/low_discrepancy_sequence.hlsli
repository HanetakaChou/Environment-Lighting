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

#ifndef _LOW_DISCREPANCY_HLSLI_
#define _LOW_DISCREPANCY_HLSLI_ 1

float2 hammersley_2d(uint sample_index, uint sample_count)
{
    // "7.4.1 Hammersley and Halton Sequences" of PBR Book
    // UE: [Hammersley](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/MonteCarlo.ush#L34)
    // U3D: [Hammersley2d](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/Sampling/Hammersley.hlsl#L415)

    float xi_1 = float(sample_index) / float(sample_count);
    float xi_2 = reversebits(sample_index) * (1.0 / 4294967296.0);

    return float2(xi_1, xi_2);
}


float2 fibonacci_2d(uint sample_index, uint sample_count)
{
    // TODO
    return float2(0.7, 0.7);
}

#endif
