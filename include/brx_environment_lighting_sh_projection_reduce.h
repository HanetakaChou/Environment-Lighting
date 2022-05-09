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

#ifndef _BRX_ENVIRONMENT_LIGHTING_SH_PROJECTION_REDUCE_H_
#define _BRX_ENVIRONMENT_LIGHTING_SH_PROJECTION_REDUCE_H_ 1

// https://docs.microsoft.com/en-us/windows/win32/direct3d11/direct3d-11-advanced-stages-compute-shader
// D3D11_CS_THREAD_GROUP_MAX_X 1024
// D3D11_CS_THREAD_GROUP_MAX_Y 1024
// D3D11_CS_THREAD_GROUP_MAX_Z 64
// D3D11_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP 1024
#define BRX_ENVIRONMENT_LIGHTING_SH_PROJECTION_THREAD_GROUP_X 16
#define BRX_ENVIRONMENT_LIGHTING_SH_PROJECTION_THREAD_GROUP_Y 16
#define BRX_ENVIRONMENT_LIGHTING_SH_PROJECTION_THREAD_GROUP_Z 1

#define BRX_ENVIRONMENT_LIGHTING_SH_PROJECTION_REDUCE_LAYOUT_UNDEFINED 0
#define BRX_ENVIRONMENT_LIGHTING_SH_PROJECTION_REDUCE_LAYOUT_EQUIRECTANGULAR_MAP 1
#define BRX_ENVIRONMENT_LIGHTING_SH_PROJECTION_REDUCE_LAYOUT_OCTAHEDRAL_MAP 2

#endif
