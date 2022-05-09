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

#include "../thirdparty/Spherical-Harmonic/include/brx_spherical_harmonic_projection_transfer_function_reduce.h"

#include <DirectXPackedVector.h>
#include <cinttypes>
#include "../thirdparty/McRT-Malloc/include/mcrt_vector.h"

static inline uintptr_t internal_brx_open(const char *path);

static inline void internal_brx_write(uintptr_t fildes, void const *buf, size_t nbyte);

static inline void internal_brx_close(uintptr_t fildes);

int main(int argc, char *argv[])
{
    constexpr uint32_t const sh_lut_width = 128U;
    constexpr uint32_t const sh_lut_height = 128U;

    std::vector<float> sh_lut_transfer_functions(static_cast<size_t>(sh_lut_width * sh_lut_height * static_cast<uint32_t>(BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT)));
    brx_spherical_harmonic_projection_transfer_functions(sh_lut_transfer_functions.data(), sh_lut_width, sh_lut_height);

    // write DDS
    {
        uintptr_t file = internal_brx_open("brx_spherical_harmonic_look_up_table_transfer_functions.dds");

        {
            uint32_t dds_metadata[] =
                {
                    // DDS_MAGIC
                    0X20534444U,
                    // sizeof(DDS_HEADER)
                    124U,
                    // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_DEPTH
                    0X801007U,
                    // Height
                    sh_lut_height,
                    // Width
                    sh_lut_width,
                    // PitchOrLinearSize
                    0U,
                    // Depth,
                    static_cast<uint32_t>(BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT),
                    // MipMapCount
                    0U,
                    // Reserved1[11]
                    0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
                    // sizeof(DDS_PIXELFORMAT)
                    32U,
                    // DDPF_FOURCC
                    0X4U,
                    // D3DFMT_R16F
                    111U,
                    // RGBBitCount
                    0U,
                    // RBitMask
                    0U,
                    // GBitMask
                    0U,
                    // BBitMask
                    0U,
                    // ABitMask
                    0U,
                    // DDSCAPS_TEXTURE
                    0X1000U,
                    // DDSCAPS2_VOLUME
                    0X200000U,
                    // Caps3
                    0U,
                    // Caps4
                    0U,
                    // Reserved2
                    0U};

            internal_brx_write(file, dds_metadata, sizeof(dds_metadata));
        }

        {
            mcrt_vector<uint16_t> half(static_cast<size_t>(sh_lut_width * sh_lut_height * static_cast<uint32_t>(BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT)));

            for (uint32_t ltc_lut_index = 0U; ltc_lut_index < (sh_lut_width * sh_lut_height * static_cast<uint32_t>(BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT)); ++ltc_lut_index)
            {
                half[ltc_lut_index] = DirectX::PackedVector::XMConvertFloatToHalf(sh_lut_transfer_functions[ltc_lut_index]);
            }

            assert((sizeof(uint16_t) * half.size()) == (sizeof(uint16_t) * sh_lut_width * sh_lut_height * static_cast<uint32_t>(BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT)));

            internal_brx_write(file, half.data(), sizeof(uint16_t) * half.size());
        }

        internal_brx_close(file);
    }

    // write C header
    {
        uintptr_t file = internal_brx_open("../thirdparty/Environment-Lighting/include/brx_spherical_harmonic_look_up_table_transfer_functions.h");

        {
            constexpr char const string[] = {"//\r\n// Copyright (C) YuqiaoZhang(HanetakaChou)\r\n//\r\n// This program is free software: you can redistribute it and/or modify\r\n// it under the terms of the GNU Lesser General Public License as published\r\n// by the Free Software Foundation, either version 3 of the License, or\r\n// (at your option) any later version.\r\n//\r\n// This program is distributed in the hope that it will be useful,\r\n// but WITHOUT ANY WARRANTY; without even the implied warranty of\r\n// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\r\n// GNU Lesser General Public License for more details.\r\n//\r\n// You should have received a copy of the GNU Lesser General Public License\r\n// along with this program.  If not, see <https://www.gnu.org/licenses/>.\r\n//\r\n\r\n#ifndef _BRX_SPHERICAL_HARMONIC_LOOK_UP_TABLE_TRANSFER_FUNCTIONS_H_\r\n#define _BRX_SPHERICAL_HARMONIC_LOOK_UP_TABLE_TRANSFER_FUNCTIONS_H_ 1\r\n\r\n// clang-format off\r\n\r\n"};

            internal_brx_write(file, string, (sizeof(string) / sizeof(string[0]) - 1U));
        }

        {
            constexpr char const format[] = {"static constexpr uint32_t const g_brx_sh_lut_width = %dU;\r\n"};

            char string[256];
            int const nchar_written = std::snprintf(string, (sizeof(string) / sizeof(string[0])), format, static_cast<int>(sh_lut_width));
            assert(nchar_written < (sizeof(string) / sizeof(string[0])));

            internal_brx_write(file, string, nchar_written);
        }

        {
            constexpr char const format[] = {"static constexpr uint32_t const g_brx_sh_lut_height = %dU;\r\n"};

            char string[256];
            int const nchar_written = std::snprintf(string, (sizeof(string) / sizeof(string[0])), format, static_cast<int>(sh_lut_height));
            assert(nchar_written < (sizeof(string) / sizeof(string[0])));

            internal_brx_write(file, string, nchar_written);
        }

        {
            constexpr char const format[] = {"static constexpr uint32_t const g_brx_sh_lut_array_size = %dU;\r\n"};

            char string[256];
            int const nchar_written = std::snprintf(string, (sizeof(string) / sizeof(string[0])), format, static_cast<int>(BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT));
            assert(nchar_written < (sizeof(string) / sizeof(string[0])));

            internal_brx_write(file, string, nchar_written);
        }

        {
            constexpr char const string[] = {"static constexpr uint16_t const g_brx_sh_lut_transfer_functions[g_brx_sh_lut_width * g_brx_sh_lut_height * g_brx_sh_lut_array_size] = {\r\n"};

            internal_brx_write(file, string, (sizeof(string) / sizeof(string[0]) - 1U));
        }

        for (uint32_t ltc_lut_index = 0U; ltc_lut_index < (sh_lut_width * sh_lut_height * static_cast<uint32_t>(BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT)); ++ltc_lut_index)
        {
            uint16_t const half = DirectX::PackedVector::XMConvertFloatToHalf(sh_lut_transfer_functions[ltc_lut_index]);

            char string[256];
            int nchar_written;
            if ((sh_lut_width * sh_lut_height * static_cast<uint32_t>(BRX_SH_PROJECTION_TRANSFER_FUNCTION_LUT_SH_COEFFICIENT_COUNT)) != (ltc_lut_index + 1U))
            {
                constexpr char const format[] = {"    0X%04" PRIX16 "U,\r\n"};

                nchar_written = std::snprintf(string, (sizeof(string) / sizeof(string[0])), format, half);
            }
            else
            {
                constexpr char const format[] = {"    0X%04" PRIX16 "U\r\n"};

                nchar_written = std::snprintf(string, (sizeof(string) / sizeof(string[0])), format, half);
            }
            assert(nchar_written < (sizeof(string) / sizeof(string[0])));

            internal_brx_write(file, string, nchar_written);
        }

        {
            constexpr char const string[] = {"};\r\n\r\n// clang-format on\r\n\r\n#endif\r\n"};
            internal_brx_write(file, string, (sizeof(string) / sizeof(string[0]) - 1U));
        }

        internal_brx_close(file);
    }

    return 0;
}

#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <sdkddkver.h>
#include <Windows.h>

static inline uintptr_t internal_brx_open(const char *path)
{
    HANDLE const file = CreateFileA(path, FILE_GENERIC_WRITE, 0U, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    assert(INVALID_HANDLE_VALUE != file);
    return reinterpret_cast<uintptr_t>(file);
}

static inline void internal_brx_write(uintptr_t fildes, void const *buf, size_t nbyte)
{
    HANDLE const file = reinterpret_cast<HANDLE>(fildes);
    assert(INVALID_HANDLE_VALUE != file);

    DWORD nbyte_written;
    BOOL result_write_file = WriteFile(file, buf, static_cast<DWORD>(nbyte), &nbyte_written, NULL);
    assert(FALSE != result_write_file);
    assert(nbyte == nbyte_written);
}

static inline void internal_brx_close(uintptr_t fildes)
{
    HANDLE const file = reinterpret_cast<HANDLE>(fildes);
    assert(INVALID_HANDLE_VALUE != file);

    BOOL result_close_handle = CloseHandle(file);
    assert(FALSE != result_close_handle);
}
