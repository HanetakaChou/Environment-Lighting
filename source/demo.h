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

#ifndef _DEMO_H_
#define _DEMO_H_ 1

#include <sdkddkver.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <windows.h>

#include <dxgi.h>
#include <d3d11.h>

#define CUBE_FACE_COUNT 6
#define MAX_CUBE_MIP_LEVEL_COUNT 8

class Demo
{
	ID3D11SamplerState *m_wrap_anisotropy_sampler;
	ID3D11SamplerState *m_clamp_point_sampler;
	ID3D11SamplerState *m_clamp_linear_sampler;

	ID3D11Texture2D *m_hdr_norms_lut;
	ID3D11ShaderResourceView *m_hdr_norms_lut_srv;

	ID3D11Texture2D *m_sh_transfer_functions_lut;
	ID3D11ShaderResourceView *m_sh_transfer_functions_lut_srv;

	ID3D11Texture2D *m_environment_equirectangular_map;
	UINT32 m_environment_equirectangular_map_width;
	UINT32 m_environment_equirectangular_map_height;
	ID3D11ShaderResourceView *m_environment_equirectangular_map_srv;

	ID3D11ComputeShader *m_equirectangular_map_to_octahedral_map_cs;

	ID3D11Texture2D *m_environment_octahedral_map;
	UINT32 m_environment_octahedral_map_width;
	UINT32 m_environment_octahedral_map_height;
	ID3D11UnorderedAccessView *m_environment_octahedral_map_uav;
	ID3D11ShaderResourceView *m_environment_octahedral_map_srv;

	ID3D11ComputeShader *m_sh_project_environment_map_clear_cs;
	ID3D11ComputeShader *m_sh_project_equirectangular_map_reduction_cs;
	ID3D11ComputeShader *m_sh_project_octahedral_map_reduction_cs;

	ID3D11Buffer *m_environment_map_sh_coefficients;
	ID3D11UnorderedAccessView *m_environment_map_sh_coefficients_uav;
	ID3D11ShaderResourceView *m_environment_map_sh_coefficients_srv;

	ID3D11InputLayout *m_forward_shading_vao;
	ID3D11VertexShader *m_forward_shading_vs;
	ID3D11PixelShader *m_forward_shading_fs;
	ID3D11RasterizerState *m_forward_shading_rs;
	ID3D11Buffer *m_forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer;
	ID3D11Buffer *m_forward_shading_pipeline_layout_global_set_object_binding_uniform_buffer;

	uint32_t m_sphere_vertex_count;
	ID3D11Buffer *m_sphere_vb_position;
	ID3D11Buffer *m_sphere_vb_varying;
	uint32_t m_sphere_index_count;
	ID3D11Buffer *m_sphere_ib;

	ID3D11RenderTargetView *m_attachment_backbuffer_rtv;
	ID3D11Texture2D *m_attachment_depth;
	ID3D11DepthStencilView *m_attachment_depth_dsv;

public:
	void Init(ID3D11Device *d3d11_device, ID3D11DeviceContext *d3d11_device_context, IDXGISwapChain *dxgi_swap_chain);
	void Tick(ID3D11Device *d3d11_device, ID3D11DeviceContext *d3d11_device_context, IDXGISwapChain *dxgi_swap_chain);
};

#endif