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

#ifndef _BRX_SPHERICAL_HARMONIC_PROJECTION_TRANSFER_FUNCTION_REDUCE_H_
#define _BRX_SPHERICAL_HARMONIC_PROJECTION_TRANSFER_FUNCTION_REDUCE_H_ 1

#include <DirectXMath.h>
#include <cmath>
#include <algorithm>
#include "../../Brioche-Shader-Language/include/brx_low_discrepancy_sequence.h"
#include "../../Brioche-Shader-Language/include/brx_brdf.h"
#include "../../McRT-Malloc/include/mcrt_parallel_map.h"
#include "../../McRT-Malloc/include/mcrt_parallel_reduce.h"
#include "brx_spherical_harmonic.h"
#include "brx_spherical_harmonic_projection_transfer_function.h"

// UE4：[128](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Source/Runtime/Renderer/Private/SystemTextures.cpp#L322)
// U3D: [4096](https://github.com/Unity-Technologies/Graphics/blob/v10.8.1/com.unity.render-pipelines.core/ShaderLibrary/ImageBasedLighting.hlsl#L340)
static constexpr uint32_t const INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_MONTE_CARLO_SAMPLE_COUNT = 16384U;

static constexpr uint32_t const INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_MONTE_CARLO_GRAIN_SIZE = 64U;

static_assert((((static_cast<uint32_t>(BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT) + 1U) * 2U - static_cast<uint32_t>(BRX_SH_COEFFICIENT_COUNT)) * ((static_cast<uint32_t>(BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT) + 1U) * 2U - static_cast<uint32_t>(BRX_SH_COEFFICIENT_COUNT))) == static_cast<uint32_t>(BRX_SH_COEFFICIENT_COUNT), "");

static constexpr float const INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_SH_COEFFICIENT_THRESHOLD = 2E-4F;

struct INTERNAL_BRX_FLOAT_SH
{
    //  0 -> (0,  0)
    //  1 -> (1, -1)
    //  2 -> (1,  0)
    //  3 -> (1,  1)
    //  4 -> (2, -2)
    //  5 -> (2, -1)
    //  6 -> (2,  0)
    //  7 -> (2,  1)
    //  8 -> (2,  2)
    //  9 -> (3, -3)
    // 10 -> (3, -2)
    // 11 -> (3, -1)
    // 12 -> (3,  0)
    // 13 -> (3,  1)
    // 14 -> (3,  2)
    // 15 -> (3,  3)
    // 16 -> (4, -4)
    // 17 -> (4, -3)
    // 18 -> (4, -2)
    // 19 -> (4, -1)
    // 20 -> (4,  0)
    // 21 -> (4,  1)
    // 22 -> (4,  2)
    // 23 -> (4,  3)
    // 24 -> (4,  4)
    // 25 -> (5, -5)
    // 26 -> (5, -4)
    // 27 -> (5, -3)
    // 28 -> (5, -2)
    // 29 -> (5, -1)
    // 30 -> (5,  0)
    // 31 -> (5,  1)
    // 32 -> (5,  2)
    // 33 -> (5,  3)
    // 34 -> (5,  4)
    // 35 -> (5,  5)
    float v[BRX_SH_COEFFICIENT_COUNT];
};

struct INTERNAL_BRX_DOUBLE_SH
{
    double v[BRX_SH_COEFFICIENT_COUNT];
};

static inline INTERNAL_BRX_FLOAT_SH internal_brx_float_sh_evaluate_basis(DirectX::XMFLOAT3 const &direction);

static inline INTERNAL_BRX_DOUBLE_SH internal_brx_float_sh_scale_double(INTERNAL_BRX_FLOAT_SH const &x, double scale);

static inline INTERNAL_BRX_DOUBLE_SH internal_brx_double_sh_zero();

static inline INTERNAL_BRX_DOUBLE_SH internal_brx_double_sh_add_double(INTERNAL_BRX_DOUBLE_SH const &x, INTERNAL_BRX_DOUBLE_SH const &y);

static inline void internal_brx_spherical_harmonic_projection_transfer_function(uint32_t const lut_width_index, uint32_t const lut_height_index, uint32_t const lut_width, uint32_t const lut_height, float *const out_transfer_function_lut_sh_coefficients);

static inline void brx_spherical_harmonic_projection_transfer_functions(float *const out_lut, uint32_t const lut_width, uint32_t const lut_height)
{
    // UE4: [128x32](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Source/Runtime/Renderer/Private/SystemTextures.cpp#L289)
    // U3D: [64x64] (https://github.com/Unity-Technologies/Graphics/blob/v10.8.1/com.unity.render-pipelines.high-definition/Runtime/Material/PreIntegratedFGD/PreIntegratedFGD.cs.hlsl#L10)

    struct parallel_map_user_data
    {
        float *const out_lut;
        uint32_t const lut_width;
        uint32_t const lut_height;
    };

    parallel_map_user_data user_data = {out_lut, lut_width, lut_height};

    mcrt_parallel_map(
        0U,
        lut_height * lut_width,
        1U,
        [](uint32_t begin, uint32_t end, void *wrapped_user_data) -> void
        {
            parallel_map_user_data *unwrapped_user_data = static_cast<parallel_map_user_data *>(wrapped_user_data);
            float *const out_lut = unwrapped_user_data->out_lut;
            uint32_t const lut_width = unwrapped_user_data->lut_width;
            uint32_t const lut_height = unwrapped_user_data->lut_height;

            assert((begin + 1U) == end);
            uint32_t const lut_width_index = (begin % lut_width);
            uint32_t const lut_height_index = (begin / lut_width);

            float transfer_function_lut_sh_coefficients[static_cast<uint32_t>(BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT)];
            internal_brx_spherical_harmonic_projection_transfer_function(lut_width_index, lut_height_index, lut_width, lut_height, transfer_function_lut_sh_coefficients);

            for (uint32_t non_zero_sh_coefficient_index = 0U; non_zero_sh_coefficient_index < static_cast<uint32_t>(BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT); ++non_zero_sh_coefficient_index)
            {
                out_lut[lut_width * lut_height * non_zero_sh_coefficient_index + lut_width * lut_height_index + lut_width_index] = transfer_function_lut_sh_coefficients[non_zero_sh_coefficient_index];
            }
        },
        &user_data);
}

static inline void internal_brx_spherical_harmonic_projection_compute_monochromatic_brdf_cosine_zeroth_spherical_moment(float const raw_alpha, DirectX::XMFLOAT3 const &raw_omega_o, float &out_norm)
{
    assert(raw_alpha >= BRX_TROWBRIDGE_REITZ_ALPHA_MINIMUM);
    float const alpha = std::max(BRX_TROWBRIDGE_REITZ_ALPHA_MINIMUM, raw_alpha);

    assert(raw_omega_o.z >= BRX_TROWBRIDGE_REITZ_NDOTV_MINIMUM);
    DirectX::XMFLOAT3 const omega_o(raw_omega_o.x, raw_omega_o.y, std::max(BRX_TROWBRIDGE_REITZ_NDOTV_MINIMUM, raw_omega_o.z));

    struct parallel_reduce_user_data
    {
        DirectX::XMFLOAT3 const omega_o;
        float const alpha;
    };

    parallel_reduce_user_data user_data = {omega_o, alpha};

    double norm = mcrt_parallel_reduce_double(
        0U,
        INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_MONTE_CARLO_SAMPLE_COUNT,
        INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_MONTE_CARLO_GRAIN_SIZE,
        [](uint32_t begin, uint32_t end, void *wrapped_user_data) -> double
        {
            parallel_reduce_user_data *unwrapped_user_data = static_cast<parallel_reduce_user_data *>(wrapped_user_data);
            DirectX::XMFLOAT3 const omega_o = unwrapped_user_data->omega_o;
            float const alpha = unwrapped_user_data->alpha;

            double norm = 0.0;

            for (uint32_t sample_index = begin; sample_index < end; ++sample_index)
            {
                DirectX::XMFLOAT2 xi = brx_hammersley_2d(sample_index, INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_MONTE_CARLO_SAMPLE_COUNT);

                DirectX::XMFLOAT3 omega_h = brx_trowbridge_reitz_sample_omega_h(xi, alpha, omega_o);

                DirectX::XMFLOAT3 omega_i;
                {
                    DirectX::XMStoreFloat3(&omega_i, DirectX::XMVector3Normalize(DirectX::XMVector3Reflect(DirectX::XMVectorNegate(DirectX::XMLoadFloat3(&omega_o)), DirectX::XMLoadFloat3(&omega_h))));
                }

                float NdotL = std::max(0.0F, omega_i.z);

                float monochromatic_throughput = brx_trowbridge_reitz_throughput_without_fresnel(alpha, NdotL);

                assert(monochromatic_throughput >= 0.0F);
                norm += ((1.0 / static_cast<double>(INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_MONTE_CARLO_SAMPLE_COUNT)) * static_cast<double>(monochromatic_throughput));
            }

            return norm;
        },
        &user_data);

    out_norm = static_cast<float>(norm);
}

static inline void internal_brx_spherical_harmonic_projection_compute_normalized_monochromatic_brdf_cosine_sh_coefficients(float const raw_alpha, DirectX::XMFLOAT3 const &raw_omega_o, float monochromatic_brdf_cosine_norm, INTERNAL_BRX_FLOAT_SH &out_normalized_monochromatic_brdf_cosine_sh_coefficients)
{
    assert(raw_alpha >= BRX_TROWBRIDGE_REITZ_ALPHA_MINIMUM);
    float const alpha = std::max(BRX_TROWBRIDGE_REITZ_ALPHA_MINIMUM, raw_alpha);

    assert(raw_omega_o.z >= BRX_TROWBRIDGE_REITZ_NDOTV_MINIMUM);
    DirectX::XMFLOAT3 const omega_o(raw_omega_o.x, raw_omega_o.y, std::max(BRX_TROWBRIDGE_REITZ_NDOTV_MINIMUM, raw_omega_o.z));

    struct parallel_reduce_user_data
    {
        DirectX::XMFLOAT3 const omega_o;
        float const alpha;
        float const monochromatic_brdf_cosine_norm;
    };

    parallel_reduce_user_data user_data = {omega_o, alpha, monochromatic_brdf_cosine_norm};

    mcrt_double36 normalized_monochromatic_brdf_cosine_double36_sh_coefficients = mcrt_parallel_reduce_double36(
        0U,
        INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_MONTE_CARLO_SAMPLE_COUNT,
        INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_MONTE_CARLO_GRAIN_SIZE,
        [](uint32_t begin, uint32_t end, void *wrapped_user_data) -> mcrt_double36
        {
            parallel_reduce_user_data *unwrapped_user_data = static_cast<parallel_reduce_user_data *>(wrapped_user_data);
            DirectX::XMFLOAT3 const omega_o = unwrapped_user_data->omega_o;
            float const alpha = unwrapped_user_data->alpha;
            float const monochromatic_brdf_cosine_norm = unwrapped_user_data->monochromatic_brdf_cosine_norm;

            INTERNAL_BRX_DOUBLE_SH normalized_monochromatic_brdf_cosine_sh_coefficients = internal_brx_double_sh_zero();

            for (uint32_t sample_index = begin; sample_index < end; ++sample_index)
            {
                DirectX::XMFLOAT2 xi = brx_hammersley_2d(sample_index, INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_MONTE_CARLO_SAMPLE_COUNT);

                DirectX::XMFLOAT3 omega_h = brx_trowbridge_reitz_sample_omega_h(xi, alpha, omega_o);

                DirectX::XMFLOAT3 omega_i;
                {
                    DirectX::XMStoreFloat3(&omega_i, DirectX::XMVector3Normalize(DirectX::XMVector3Reflect(DirectX::XMVectorNegate(DirectX::XMLoadFloat3(&omega_o)), DirectX::XMLoadFloat3(&omega_h))));
                }

                float NdotL = std::max(0.0F, omega_i.z);

                float monochromatic_throughput = brx_trowbridge_reitz_throughput_without_fresnel(alpha, NdotL);

                INTERNAL_BRX_FLOAT_SH upsilon = internal_brx_float_sh_evaluate_basis(omega_i);

                assert(monochromatic_throughput >= 0.0F);
                normalized_monochromatic_brdf_cosine_sh_coefficients = internal_brx_double_sh_add_double(normalized_monochromatic_brdf_cosine_sh_coefficients, internal_brx_float_sh_scale_double(upsilon, (1.0 / static_cast<double>(INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_MONTE_CARLO_SAMPLE_COUNT)) * static_cast<double>(monochromatic_throughput) * (1.0 / static_cast<double>(monochromatic_brdf_cosine_norm))));
            }

            mcrt_double36 normalized_monochromatic_brdf_cosine_double36_sh_coefficients;
            static_assert((sizeof(normalized_monochromatic_brdf_cosine_double36_sh_coefficients.v) / sizeof(normalized_monochromatic_brdf_cosine_double36_sh_coefficients.v[0])) == (sizeof(normalized_monochromatic_brdf_cosine_sh_coefficients.v) / sizeof(normalized_monochromatic_brdf_cosine_sh_coefficients.v[0])), "");
            static_assert((sizeof(normalized_monochromatic_brdf_cosine_double36_sh_coefficients.v) / sizeof(normalized_monochromatic_brdf_cosine_double36_sh_coefficients.v[0])) == static_cast<uint32_t>(BRX_SH_COEFFICIENT_COUNT), "");
            for (uint32_t sh_coefficient_index = 0U; sh_coefficient_index < static_cast<uint32_t>(BRX_SH_COEFFICIENT_COUNT); ++sh_coefficient_index)
            {
                normalized_monochromatic_brdf_cosine_double36_sh_coefficients.v[sh_coefficient_index] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[sh_coefficient_index];
            }
            return normalized_monochromatic_brdf_cosine_double36_sh_coefficients;
        },
        &user_data);

    static_assert((sizeof(normalized_monochromatic_brdf_cosine_double36_sh_coefficients.v) / sizeof(normalized_monochromatic_brdf_cosine_double36_sh_coefficients.v[0])) == (sizeof(out_normalized_monochromatic_brdf_cosine_sh_coefficients.v) / sizeof(out_normalized_monochromatic_brdf_cosine_sh_coefficients.v[0])), "");
    static_assert((sizeof(normalized_monochromatic_brdf_cosine_double36_sh_coefficients.v) / sizeof(normalized_monochromatic_brdf_cosine_double36_sh_coefficients.v[0])) == BRX_SH_COEFFICIENT_COUNT, "");
    for (uint32_t sh_coefficient_index = 0U; sh_coefficient_index < static_cast<uint32_t>(BRX_SH_COEFFICIENT_COUNT); ++sh_coefficient_index)
    {
        out_normalized_monochromatic_brdf_cosine_sh_coefficients.v[sh_coefficient_index] = static_cast<float>(normalized_monochromatic_brdf_cosine_double36_sh_coefficients.v[sh_coefficient_index]);
    }
}

static inline void internal_brx_spherical_harmonic_projection_transfer_function(uint32_t const lut_width_index, uint32_t const lut_height_index, uint32_t const lut_width, uint32_t const lut_height, float *const out_transfer_function_lut_sh_coefficients)
{
    // Remap: [0, 1] -> [0.5/size, 1.0 - 0.5/size]
    // U3D: [Remap01ToHalfTexelCoord](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/Common.hlsl#L661)
    // UE4: [N/A](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/RectLight.ush#L450)

    assert(lut_width_index < lut_width);
    float texcoord_u = static_cast<float>(lut_width_index) / static_cast<float>(lut_width - 1U);

    assert(lut_height_index < lut_height);
    float texcoord_v = static_cast<float>(lut_height_index) / static_cast<float>(lut_height - 1U);

    // u = 1 - cos_theta
    // cos_theta = 1 - u
    DirectX::XMFLOAT3 omega_o;
    {
        float const cos_theta_o = std::max(BRX_TROWBRIDGE_REITZ_NDOTV_MINIMUM, 1.0F - texcoord_u);
        omega_o = DirectX::XMFLOAT3(std::sqrt(std::max(0.0F, 1.0F - cos_theta_o * cos_theta_o)), 0.0F, cos_theta_o);
    }

    // v = 1 - alpha
    // alpha = 1 - v
    float const alpha = std::max(BRX_TROWBRIDGE_REITZ_ALPHA_MINIMUM, 1.0F - texcoord_v);

    float norm;
    internal_brx_spherical_harmonic_projection_compute_monochromatic_brdf_cosine_zeroth_spherical_moment(alpha, omega_o, norm);

    INTERNAL_BRX_FLOAT_SH normalized_monochromatic_brdf_cosine_sh_coefficients;
    internal_brx_spherical_harmonic_projection_compute_normalized_monochromatic_brdf_cosine_sh_coefficients(alpha, omega_o, norm, normalized_monochromatic_brdf_cosine_sh_coefficients);

    static_assert((sizeof(normalized_monochromatic_brdf_cosine_sh_coefficients.v) / sizeof(normalized_monochromatic_brdf_cosine_sh_coefficients.v[0])) == 36U, "");

    // Constant
    // 0 -> (0, 0)
    assert(std::abs(normalized_monochromatic_brdf_cosine_sh_coefficients.v[0] - 0.28209479177387814347403972578039F) < INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_SH_COEFFICIENT_THRESHOLD);

    // For isotropic BRDF, all m < 0 must be zero
    //  1 -> (1, -1)
    //  4 -> (2, -2)
    //  5 -> (2, -1)
    //  9 -> (3, -3)
    // 10 -> (3, -2)
    // 11 -> (3, -1)
    // 16 -> (4, -4)
    // 17 -> (4, -3)
    // 18 -> (4, -2)
    // 19 -> (4, -1)
    // 25 -> (5, -5)
    // 26 -> (5, -4)
    // 27 -> (5, -3)
    // 28 -> (5, -2)
    // 29 -> (5, -1)
    assert(std::abs(normalized_monochromatic_brdf_cosine_sh_coefficients.v[1]) < INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_SH_COEFFICIENT_THRESHOLD);
    assert(std::abs(normalized_monochromatic_brdf_cosine_sh_coefficients.v[4]) < INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_SH_COEFFICIENT_THRESHOLD);
    assert(std::abs(normalized_monochromatic_brdf_cosine_sh_coefficients.v[5]) < INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_SH_COEFFICIENT_THRESHOLD);
    assert(std::abs(normalized_monochromatic_brdf_cosine_sh_coefficients.v[9]) < INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_SH_COEFFICIENT_THRESHOLD);
    assert(std::abs(normalized_monochromatic_brdf_cosine_sh_coefficients.v[10]) < INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_SH_COEFFICIENT_THRESHOLD);
    assert(std::abs(normalized_monochromatic_brdf_cosine_sh_coefficients.v[11]) < INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_SH_COEFFICIENT_THRESHOLD);
    assert(std::abs(normalized_monochromatic_brdf_cosine_sh_coefficients.v[16]) < INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_SH_COEFFICIENT_THRESHOLD);
    assert(std::abs(normalized_monochromatic_brdf_cosine_sh_coefficients.v[17]) < INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_SH_COEFFICIENT_THRESHOLD);
    assert(std::abs(normalized_monochromatic_brdf_cosine_sh_coefficients.v[18]) < INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_SH_COEFFICIENT_THRESHOLD);
    assert(std::abs(normalized_monochromatic_brdf_cosine_sh_coefficients.v[19]) < INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_SH_COEFFICIENT_THRESHOLD);
    assert(std::abs(normalized_monochromatic_brdf_cosine_sh_coefficients.v[25]) < INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_SH_COEFFICIENT_THRESHOLD);
    assert(std::abs(normalized_monochromatic_brdf_cosine_sh_coefficients.v[26]) < INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_SH_COEFFICIENT_THRESHOLD);
    assert(std::abs(normalized_monochromatic_brdf_cosine_sh_coefficients.v[27]) < INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_SH_COEFFICIENT_THRESHOLD);
    assert(std::abs(normalized_monochromatic_brdf_cosine_sh_coefficients.v[28]) < INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_SH_COEFFICIENT_THRESHOLD);
    assert(std::abs(normalized_monochromatic_brdf_cosine_sh_coefficients.v[29]) < INTERNAL_BRX_SH_PROJECTION_TRANSFER_FUNCTION_SH_COEFFICIENT_THRESHOLD);

    //  2 -> (1,  0) ->  0
    //  3 -> (1,  1) ->  1
    //  6 -> (2,  0) ->  2
    //  7 -> (2,  1) ->  3
    //  8 -> (2,  2) ->  4
    // 12 -> (3,  0) ->  5
    // 13 -> (3,  1) ->  6
    // 14 -> (3,  2) ->  7
    // 15 -> (3,  3) ->  8
    // 20 -> (4,  0) ->  9
    // 21 -> (4,  1) -> 10
    // 22 -> (4,  2) -> 11
    // 23 -> (4,  3) -> 12
    // 24 -> (4,  4) -> 13
    // 30 -> (5,  0) -> 14
    // 31 -> (5,  1) -> 15
    // 32 -> (5,  2) -> 16
    // 33 -> (5,  3) -> 17
    // 34 -> (5,  4) -> 18
    // 35 -> (5,  5) -> 19
    static_assert(static_cast<uint32_t>(BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT) == 20U, "");
    out_transfer_function_lut_sh_coefficients[0] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[2];
    out_transfer_function_lut_sh_coefficients[1] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[3];
    out_transfer_function_lut_sh_coefficients[2] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[6];
    out_transfer_function_lut_sh_coefficients[3] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[7];
    out_transfer_function_lut_sh_coefficients[4] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[8];
    out_transfer_function_lut_sh_coefficients[5] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[12];
    out_transfer_function_lut_sh_coefficients[6] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[13];
    out_transfer_function_lut_sh_coefficients[7] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[14];
    out_transfer_function_lut_sh_coefficients[8] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[15];
    out_transfer_function_lut_sh_coefficients[9] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[20];
    out_transfer_function_lut_sh_coefficients[10] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[21];
    out_transfer_function_lut_sh_coefficients[11] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[22];
    out_transfer_function_lut_sh_coefficients[12] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[23];
    out_transfer_function_lut_sh_coefficients[13] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[24];
    out_transfer_function_lut_sh_coefficients[14] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[30];
    out_transfer_function_lut_sh_coefficients[15] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[31];
    out_transfer_function_lut_sh_coefficients[16] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[32];
    out_transfer_function_lut_sh_coefficients[17] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[33];
    out_transfer_function_lut_sh_coefficients[18] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[34];
    out_transfer_function_lut_sh_coefficients[19] = normalized_monochromatic_brdf_cosine_sh_coefficients.v[35];
}

static inline INTERNAL_BRX_FLOAT_SH internal_brx_float_sh_evaluate_basis(DirectX::XMFLOAT3 const &direction)
{
    // "Appendix A2" of [Sloan 2008]: polynomial form of SH basis
    // DirectXMath: [sh_eval_basis_4](https://github.com/microsoft/DirectXMath/blob/jul2018b/SHMath/DirectXSH.cpp#L250)

    // NOTE: direction should be normalized before using the "polynomial form"
    assert(std::abs(DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMLoadFloat3(&direction))) - 1.0F) < 1E-5F);

    INTERNAL_BRX_FLOAT_SH upsilon;

    float z2 = direction.z * direction.z;

    float p_0_0 = (0.282094791773878140F);
    upsilon.v[0] = p_0_0;
    float p_1_0 = (0.488602511902919920F) * direction.z;
    upsilon.v[2] = p_1_0;
    float p_2_0 = (0.946174695757560080F) * z2 + (-0.315391565252520050F);
    upsilon.v[6] = p_2_0;
    float p_3_0 = direction.z * ((1.865881662950577000F) * z2 + (-1.119528997770346200F));
    upsilon.v[12] = p_3_0;
    float p_4_0 = (1.984313483298443000F) * direction.z * p_3_0 + (-1.006230589874905300F) * p_2_0;
    upsilon.v[20] = p_4_0;
    float p_5_0 = (1.989974874213239700F) * direction.z * p_4_0 + (-1.002853072844814000F) * p_3_0;
    upsilon.v[30] = p_5_0;

    float s1 = direction.y;
    float c1 = direction.x;
    float p_1_1 = (-0.488602511902919920F);
    upsilon.v[1] = p_1_1 * s1;
    upsilon.v[3] = p_1_1 * c1;
    float p_2_1 = (-1.092548430592079200F) * direction.z;
    upsilon.v[5] = p_2_1 * s1;
    upsilon.v[7] = p_2_1 * c1;
    float p_3_1 = (-2.285228997322328800F) * z2 + (0.457045799464465770F);
    upsilon.v[11] = p_3_1 * s1;
    upsilon.v[13] = p_3_1 * c1;
    float p_4_1 = direction.z * ((-4.683325804901024000F) * z2 + (2.007139630671867200F));
    upsilon.v[19] = p_4_1 * s1;
    upsilon.v[21] = p_4_1 * c1;
    float p_5_1 = (2.031009601158990200F) * direction.z * p_4_1 + (-0.991031208965114650F) * p_3_1;
    upsilon.v[29] = p_5_1 * s1;
    upsilon.v[31] = p_5_1 * c1;

    float s2 = direction.x * s1 + direction.y * c1;
    float c2 = direction.x * c1 - direction.y * s1;
    float p_2_2 = (0.546274215296039590F);
    upsilon.v[4] = p_2_2 * s2;
    upsilon.v[8] = p_2_2 * c2;
    float p_3_2 = (1.445305721320277100F) * direction.z;
    upsilon.v[10] = p_3_2 * s2;
    upsilon.v[14] = p_3_2 * c2;
    float p_4_2 = (3.311611435151459800F) * z2 + (-0.473087347878779980F);
    upsilon.v[18] = p_4_2 * s2;
    upsilon.v[22] = p_4_2 * c2;
    float p_5_2 = direction.z * ((7.190305177459987500F) * z2 + (-2.396768392486662100F));
    upsilon.v[28] = p_5_2 * s2;
    upsilon.v[32] = p_5_2 * c2;

    float s3 = direction.x * s2 + direction.y * c2;
    float c3 = direction.x * c2 - direction.y * s2;
    float p_3_3 = (-0.590043589926643520F);
    upsilon.v[9] = p_3_3 * s3;
    upsilon.v[15] = p_3_3 * c3;
    float p_4_3 = (-1.770130769779930200F) * direction.z;
    upsilon.v[17] = p_4_3 * s3;
    upsilon.v[23] = p_4_3 * c3;
    float p_5_3 = (-4.403144694917253700F) * z2 + (0.489238299435250430F);
    upsilon.v[27] = p_5_3 * s3;
    upsilon.v[33] = p_5_3 * c3;

    float s4 = direction.x * s3 + direction.y * c3;
    float c4 = direction.x * c3 - direction.y * s3;
    float p_4_4 = (0.625835735449176030F);
    upsilon.v[16] = p_4_4 * s4;
    upsilon.v[24] = p_4_4 * c4;
    float p_5_4 = (2.075662314881041100F) * direction.z;
    upsilon.v[26] = p_5_4 * s4;
    upsilon.v[34] = p_5_4 * c4;

    float s5 = direction.x * s4 + direction.y * c4;
    float c5 = direction.x * c4 - direction.y * s4;
    float p_5_5 = (-0.656382056840170150F);
    upsilon.v[25] = p_5_5 * s5;
    upsilon.v[35] = p_5_5 * c5;

    return upsilon;
}

static inline INTERNAL_BRX_DOUBLE_SH internal_brx_float_sh_scale_double(INTERNAL_BRX_FLOAT_SH const &x, double scale)
{
    // DirectXMath: [DirectX::XMSHScale](https://github.com/microsoft/DirectXMath/blob/jul2018b/SHMath/DirectXSH.cpp#L1363)

    INTERNAL_BRX_DOUBLE_SH y;

    for (uint32_t sh_coefficient_index = 0U; sh_coefficient_index < static_cast<uint32_t>(BRX_SH_COEFFICIENT_COUNT); ++sh_coefficient_index)
    {
        y.v[sh_coefficient_index] = static_cast<double>(x.v[sh_coefficient_index]) * scale;
    }

    return y;
}

static inline INTERNAL_BRX_DOUBLE_SH internal_brx_double_sh_zero()
{
    INTERNAL_BRX_DOUBLE_SH x;

    for (uint32_t sh_coefficient_index = 0U; sh_coefficient_index < static_cast<uint32_t>(BRX_SH_COEFFICIENT_COUNT); ++sh_coefficient_index)
    {
        x.v[sh_coefficient_index] = 0.0;
    }

    return x;
}

static inline INTERNAL_BRX_DOUBLE_SH internal_brx_double_sh_add_double(INTERNAL_BRX_DOUBLE_SH const &x, INTERNAL_BRX_DOUBLE_SH const &y)
{
    // DirectXMath: [DirectX::XMSHAdd](https://github.com/microsoft/DirectXMath/blob/jul2018b/SHMath/DirectXSH.cpp#L1337)

    INTERNAL_BRX_DOUBLE_SH z;

    for (uint32_t sh_coefficient_index = 0U; sh_coefficient_index < static_cast<uint32_t>(BRX_SH_COEFFICIENT_COUNT); ++sh_coefficient_index)
    {
        z.v[sh_coefficient_index] = x.v[sh_coefficient_index] + y.v[sh_coefficient_index];
    }

    return z;
}

#endif
