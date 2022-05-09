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

#ifndef _INTEGRATE_LD_NORMAL_DISTRIBUTION_PIPELINE_LAYOUT_H_
#define _INTEGRATE_LD_NORMAL_DISTRIBUTION_PIPELINE_LAYOUT_H_ 1

#if defined(__STDC__) || defined(__cplusplus)

struct integrate_ld_normal_distribution_pipeline_layout_global_set_distribution_binding_uniform_buffer
{
	uint32_t LD_Normal_Distribution_MipLevelCount;
	uint32_t __padding_align16_1;
	uint32_t __padding_align16_2;
	uint32_t __padding_align16_3;
};

#elif defined(HLSL_VERSION) || defined(__HLSL_VERSION)

cbuffer integrate_ld_normal_distribution_pipeline_layout_global_set_distribution_binding_uniform_buffer : register(b0)
{
	uint LD_Normal_Distribution_MipLevelCount;
};

#else
#error Unknown Compiler
#endif

#endif