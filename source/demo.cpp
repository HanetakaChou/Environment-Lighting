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

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#include <sdkddkver.h>
#include <windows.h>
#include <stdint.h>
#include <assert.h>
#include <cmath>
#include <algorithm>
#include <DirectXMath.h>
#include <dxgi.h>
#include <d3d11.h>
#include "resolution.h"
#include "camera_controller.h"
#include "DDSTextureLoader.h"
#include "demo.h"

#include "../thirdparty/Spherical-Harmonic/include/brx_spherical_harmonic.h"
#include "../thirdparty/Spherical-Harmonic/include/brx_spherical_harmonic_projection_environment_map_reduce.h"
#include "../shaders/forward_shading_pipeline_layout.h"
#ifndef NDEBUG
#include "../shaders/dxbc/debug/equirectangular_map_to_octahedral_map.inl"
#include "../shaders/dxbc/debug/environment_lighting_sh_projection_clear.inl"
#include "../shaders/dxbc/debug/environment_lighting_sh_projection_equirectangular_map.inl"
#include "../shaders/dxbc/debug/environment_lighting_sh_projection_octahedral_map.inl"
#include "../shaders/dxbc/debug/forward_shading_vertex.inl"
#include "../shaders/dxbc/debug/forward_shading_fragment.inl"
#else
#include "../shaders/dxbc/release/equirectangular_map_to_octahedral_map.inl"
#include "../shaders/dxbc/release/environment_lighting_sh_projection_clear.inl"
#include "../shaders/dxbc/release/environment_lighting_sh_projection_equirectangular_map.inl"
#include "../shaders/dxbc/release/environment_lighting_sh_projection_octahedral_map.inl"
#include "../shaders/dxbc/release/forward_shading_vertex.inl"
#include "../shaders/dxbc/release/forward_shading_fragment.inl"
#endif
#include "../assets/sphere.h"
#include "../thirdparty/Hemispherical-Directional-Reflectance/include/brx_hemispherical_directional_reflectance_look_up_table_norms.h"
#include "../thirdparty/Spherical-Harmonic/include/brx_spherical_harmonic_look_up_table_transfer_functions.h"

static inline int8_t float_to_snorm(float unpacked_input);

void Demo::Init(ID3D11Device *d3d_device, ID3D11DeviceContext *d3d_device_context, IDXGISwapChain *dxgi_swap_chain)
{
	this->m_wrap_anisotropy_sampler = NULL;
	{
		D3D11_SAMPLER_DESC d3d_sampler_desc;
		d3d_sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
		d3d_sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		d3d_sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		d3d_sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		d3d_sampler_desc.MipLODBias = 0.0;
		d3d_sampler_desc.MaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
		d3d_sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		d3d_sampler_desc.BorderColor[0] = 0.0;
		d3d_sampler_desc.BorderColor[1] = 0.0;
		d3d_sampler_desc.BorderColor[2] = 0.0;
		d3d_sampler_desc.BorderColor[3] = 1.0;
		d3d_sampler_desc.MinLOD = 0.0;
		d3d_sampler_desc.MaxLOD = 4096.0;

		HRESULT res_d3d_device_create_sampler = d3d_device->CreateSamplerState(&d3d_sampler_desc, &this->m_wrap_anisotropy_sampler);
		assert(SUCCEEDED(res_d3d_device_create_sampler));
	}

	this->m_clamp_point_sampler = NULL;
	{
		D3D11_SAMPLER_DESC d3d_sampler_desc;
		d3d_sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		d3d_sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		d3d_sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		d3d_sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		d3d_sampler_desc.MipLODBias = 0.0;
		d3d_sampler_desc.MaxAnisotropy = 0U;
		d3d_sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		d3d_sampler_desc.BorderColor[0] = 0.0;
		d3d_sampler_desc.BorderColor[1] = 0.0;
		d3d_sampler_desc.BorderColor[2] = 0.0;
		d3d_sampler_desc.BorderColor[3] = 1.0;
		d3d_sampler_desc.MinLOD = 0.0;
		d3d_sampler_desc.MaxLOD = 4096.0;

		HRESULT res_d3d_device_create_sampler = d3d_device->CreateSamplerState(&d3d_sampler_desc, &this->m_clamp_point_sampler);
		assert(SUCCEEDED(res_d3d_device_create_sampler));
	}

	this->m_clamp_linear_sampler = NULL;
	{
		D3D11_SAMPLER_DESC d3d_sampler_desc;
		d3d_sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		d3d_sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		d3d_sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		d3d_sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		d3d_sampler_desc.MipLODBias = 0.0;
		d3d_sampler_desc.MaxAnisotropy = 0U;
		d3d_sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		d3d_sampler_desc.BorderColor[0] = 0.0;
		d3d_sampler_desc.BorderColor[1] = 0.0;
		d3d_sampler_desc.BorderColor[2] = 0.0;
		d3d_sampler_desc.BorderColor[3] = 1.0;
		d3d_sampler_desc.MinLOD = 0.0;
		d3d_sampler_desc.MaxLOD = 4096.0;

		HRESULT res_d3d_device_create_sampler = d3d_device->CreateSamplerState(&d3d_sampler_desc, &this->m_clamp_linear_sampler);
		assert(SUCCEEDED(res_d3d_device_create_sampler));
	}

	this->m_hdr_norms_lut = NULL;
	{
		static_assert((2U * g_brx_hdr_lut_width * g_brx_hdr_lut_height) == (sizeof(g_brx_hdr_lut_norms) / sizeof(g_brx_hdr_lut_norms[0])), "");

		D3D11_TEXTURE2D_DESC d3d_texture2d_desc;
		d3d_texture2d_desc.Width = g_brx_hdr_lut_width;
		d3d_texture2d_desc.Height = g_brx_hdr_lut_height;
		d3d_texture2d_desc.MipLevels = 1U;
		d3d_texture2d_desc.ArraySize = 1U;
		d3d_texture2d_desc.Format = DXGI_FORMAT_R16G16_FLOAT;
		d3d_texture2d_desc.SampleDesc.Count = 1U;
		d3d_texture2d_desc.SampleDesc.Quality = 0U;
		d3d_texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_texture2d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		d3d_texture2d_desc.CPUAccessFlags = 0U;
		d3d_texture2d_desc.MiscFlags = 0U;

		D3D11_SUBRESOURCE_DATA d3d_subresource_data;
		d3d_subresource_data.pSysMem = g_brx_hdr_lut_norms;
		d3d_subresource_data.SysMemPitch = sizeof(uint16_t) * 2U * g_brx_hdr_lut_width;
		d3d_subresource_data.SysMemSlicePitch = sizeof(uint16_t) * 2U * g_brx_hdr_lut_width * g_brx_hdr_lut_height;

		HRESULT res_d3d_device_create_texture = d3d_device->CreateTexture2D(&d3d_texture2d_desc, &d3d_subresource_data, &this->m_hdr_norms_lut);
		assert(SUCCEEDED(res_d3d_device_create_texture));
	}

	this->m_hdr_norms_lut_srv = NULL;
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC d3d_shader_resource_view_desc;
		d3d_shader_resource_view_desc.Format = DXGI_FORMAT_R16G16_FLOAT;
		d3d_shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		d3d_shader_resource_view_desc.Texture2D.MostDetailedMip = 0U;
		d3d_shader_resource_view_desc.Texture2D.MipLevels = 1U;

		HRESULT res_d3d_device_create_shader_resource_view = d3d_device->CreateShaderResourceView(this->m_hdr_norms_lut, &d3d_shader_resource_view_desc, &this->m_hdr_norms_lut_srv);
		assert(SUCCEEDED(res_d3d_device_create_shader_resource_view));
	}

	this->m_sh_transfer_functions_lut = NULL;
	{
		static_assert((g_brx_sh_lut_width * g_brx_sh_lut_height * g_brx_sh_lut_array_size) == (sizeof(g_brx_sh_lut_transfer_functions) / sizeof(g_brx_sh_lut_transfer_functions[0])), "");

		D3D11_TEXTURE2D_DESC d3d_texture2d_desc;
		d3d_texture2d_desc.Width = g_brx_sh_lut_width;
		d3d_texture2d_desc.Height = g_brx_sh_lut_height;
		d3d_texture2d_desc.MipLevels = 1U;
		d3d_texture2d_desc.ArraySize = g_brx_sh_lut_array_size;
		d3d_texture2d_desc.Format = DXGI_FORMAT_R16_FLOAT;
		d3d_texture2d_desc.SampleDesc.Count = 1U;
		d3d_texture2d_desc.SampleDesc.Quality = 0U;
		d3d_texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_texture2d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		d3d_texture2d_desc.CPUAccessFlags = 0U;
		d3d_texture2d_desc.MiscFlags = 0U;

		D3D11_SUBRESOURCE_DATA d3d_subresource_data[g_brx_sh_lut_array_size];
		for (uint32_t sh_lut_array_index = 0U; sh_lut_array_index < g_brx_sh_lut_array_size; ++sh_lut_array_index)
		{
			d3d_subresource_data[sh_lut_array_index].pSysMem = g_brx_sh_lut_transfer_functions + g_brx_sh_lut_width * g_brx_sh_lut_height * sh_lut_array_index;
			d3d_subresource_data[sh_lut_array_index].SysMemPitch = sizeof(uint16_t) * g_brx_sh_lut_width;
			d3d_subresource_data[sh_lut_array_index].SysMemSlicePitch = sizeof(uint16_t) * g_brx_sh_lut_width * g_brx_sh_lut_height;
		}

		HRESULT res_d3d_device_create_texture = d3d_device->CreateTexture2D(&d3d_texture2d_desc, d3d_subresource_data, &this->m_sh_transfer_functions_lut);
		assert(SUCCEEDED(res_d3d_device_create_texture));
	}

	this->m_sh_transfer_functions_lut_srv = NULL;
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC d3d_shader_resource_view_desc;
		d3d_shader_resource_view_desc.Format = DXGI_FORMAT_R16_FLOAT;
		d3d_shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		d3d_shader_resource_view_desc.Texture2DArray.MostDetailedMip = 0U;
		d3d_shader_resource_view_desc.Texture2DArray.MipLevels = 1U;
		d3d_shader_resource_view_desc.Texture2DArray.FirstArraySlice = 0U;
		d3d_shader_resource_view_desc.Texture2DArray.ArraySize = g_brx_sh_lut_array_size;

		HRESULT res_d3d_device_create_shader_resource_view = d3d_device->CreateShaderResourceView(this->m_sh_transfer_functions_lut, &d3d_shader_resource_view_desc, &this->m_sh_transfer_functions_lut_srv);
		assert(SUCCEEDED(res_d3d_device_create_shader_resource_view));
	}

	this->m_environment_equirectangular_map_srv = NULL;
	{
		ID3D11Resource *tmp = NULL;

		HRESULT res_create_dds_texture_from_file = DirectX::CreateDDSTextureFromFileEx(d3d_device, L"the-sky-is-on-fire-equirectangular.dds", 0U, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0U, 0U, true, &tmp, &this->m_environment_equirectangular_map_srv);
		assert(SUCCEEDED(res_create_dds_texture_from_file));

		HRESULT res_query_interface = tmp->QueryInterface(IID_PPV_ARGS(&this->m_environment_equirectangular_map));
		assert(SUCCEEDED(res_query_interface));

		D3D11_TEXTURE2D_DESC desc;
		this->m_environment_equirectangular_map->GetDesc(&desc);
		this->m_environment_equirectangular_map_width = desc.Width;
		this->m_environment_equirectangular_map_height = desc.Height;

		tmp->Release();
	}

	this->m_equirectangular_map_to_octahedral_map_cs = NULL;
	{
		HRESULT res_d3d_device_create_compute_shader = d3d_device->CreateComputeShader(code_shader_equirectangular_map_to_octahedral_map, sizeof(code_shader_equirectangular_map_to_octahedral_map), NULL, &this->m_equirectangular_map_to_octahedral_map_cs);
		assert(SUCCEEDED(res_d3d_device_create_compute_shader));
	}

	this->m_environment_octahedral_map = NULL;
	{
		this->m_environment_octahedral_map_width = std::max(this->m_environment_equirectangular_map_width, this->m_environment_equirectangular_map_height);
		this->m_environment_octahedral_map_height = std::max(this->m_environment_equirectangular_map_width, this->m_environment_equirectangular_map_height);

		D3D11_TEXTURE2D_DESC d3d_texture2d_desc;
		d3d_texture2d_desc.Width = this->m_environment_octahedral_map_width;
		d3d_texture2d_desc.Height = this->m_environment_octahedral_map_height;
		d3d_texture2d_desc.MipLevels = 1U;
		d3d_texture2d_desc.ArraySize = 1U;
		d3d_texture2d_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		d3d_texture2d_desc.SampleDesc.Count = 1U;
		d3d_texture2d_desc.SampleDesc.Quality = 0U;
		d3d_texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_texture2d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		d3d_texture2d_desc.CPUAccessFlags = 0U;
		d3d_texture2d_desc.MiscFlags = 0U;

		HRESULT res_d3d_device_create_texture = d3d_device->CreateTexture2D(&d3d_texture2d_desc, NULL, &this->m_environment_octahedral_map);
		assert(SUCCEEDED(res_d3d_device_create_texture));
	}

	this->m_environment_octahedral_map_uav = NULL;
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC d3d_unordered_access_view_desc;
		d3d_unordered_access_view_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		d3d_unordered_access_view_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		d3d_unordered_access_view_desc.Texture2D.MipSlice = 0U;

		HRESULT res_d3d_device_create_unordered_access_view = d3d_device->CreateUnorderedAccessView(this->m_environment_octahedral_map, &d3d_unordered_access_view_desc, &this->m_environment_octahedral_map_uav);
		assert(SUCCEEDED(res_d3d_device_create_unordered_access_view));
	}

	this->m_environment_octahedral_map_srv = NULL;
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC d3d_shader_resource_view_desc;
		d3d_shader_resource_view_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		d3d_shader_resource_view_desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
		d3d_shader_resource_view_desc.Texture2D.MostDetailedMip = 0U;
		d3d_shader_resource_view_desc.Texture2D.MipLevels = 1U;
		HRESULT res_d3d_device_create_shader_resource_view = d3d_device->CreateShaderResourceView(this->m_environment_octahedral_map, &d3d_shader_resource_view_desc, &this->m_environment_octahedral_map_srv);
		assert(SUCCEEDED(res_d3d_device_create_shader_resource_view));
	}

	this->m_sh_project_environment_map_clear_cs = NULL;
	{
		HRESULT res_d3d_device_create_compute_shader = d3d_device->CreateComputeShader(code_shader_environment_lighting_sh_projection_clear, sizeof(code_shader_environment_lighting_sh_projection_clear), NULL, &this->m_sh_project_environment_map_clear_cs);
		assert(SUCCEEDED(res_d3d_device_create_compute_shader));
	}

	this->m_sh_project_equirectangular_map_reduction_cs = NULL;
	{
		HRESULT res_d3d_device_create_compute_shader = d3d_device->CreateComputeShader(code_shader_environment_lighting_sh_projection_equirectangular_map, sizeof(code_shader_environment_lighting_sh_projection_equirectangular_map), NULL, &this->m_sh_project_equirectangular_map_reduction_cs);
		assert(SUCCEEDED(res_d3d_device_create_compute_shader));
	}

	this->m_sh_project_octahedral_map_reduction_cs = NULL;
	{
		HRESULT res_d3d_device_create_compute_shader = d3d_device->CreateComputeShader(code_shader_environment_lighting_sh_projection_octahedral_map, sizeof(code_shader_environment_lighting_sh_projection_octahedral_map), NULL, &this->m_sh_project_octahedral_map_reduction_cs);
		assert(SUCCEEDED(res_d3d_device_create_compute_shader));
	}

	this->m_environment_map_sh_coefficients = NULL;
	{

		D3D11_BUFFER_DESC d3d_buffer_desc;
		d3d_buffer_desc.ByteWidth = sizeof(float) * 3U * static_cast<uint32_t>(BRX_SH_COEFFICIENT_COUNT);
		d3d_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		d3d_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		d3d_buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		d3d_buffer_desc.StructureByteStride = 0U;
		HRESULT res_d3d_device_create_buffer = d3d_device->CreateBuffer(&d3d_buffer_desc, NULL, &this->m_environment_map_sh_coefficients);
		assert(SUCCEEDED(res_d3d_device_create_buffer));
	}

	this->m_environment_map_sh_coefficients_uav = NULL;
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC d3d_unordered_access_view_desc;
		d3d_unordered_access_view_desc.Format = DXGI_FORMAT_R32_TYPELESS;
		d3d_unordered_access_view_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		d3d_unordered_access_view_desc.Buffer.FirstElement = 0U;
		d3d_unordered_access_view_desc.Buffer.NumElements = 3U * static_cast<uint32_t>(BRX_SH_COEFFICIENT_COUNT);
		d3d_unordered_access_view_desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
		HRESULT res_d3d_device_create_unordered_access_view = d3d_device->CreateUnorderedAccessView(this->m_environment_map_sh_coefficients, &d3d_unordered_access_view_desc, &this->m_environment_map_sh_coefficients_uav);
		assert(SUCCEEDED(res_d3d_device_create_unordered_access_view));
	}

	this->m_environment_map_sh_coefficients_srv = NULL;
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC d3d_shader_resource_view_desc;
		d3d_shader_resource_view_desc.Format = DXGI_FORMAT_R32_TYPELESS;
		d3d_shader_resource_view_desc.ViewDimension = D3D_SRV_DIMENSION_BUFFEREX;
		d3d_shader_resource_view_desc.BufferEx.FirstElement = 0U;
		d3d_shader_resource_view_desc.BufferEx.NumElements = 3U * static_cast<uint32_t>(BRX_SH_COEFFICIENT_COUNT);
		d3d_shader_resource_view_desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
		HRESULT res_d3d_device_create_shader_resource_view = d3d_device->CreateShaderResourceView(this->m_environment_map_sh_coefficients, &d3d_shader_resource_view_desc, &this->m_environment_map_sh_coefficients_srv);
		assert(SUCCEEDED(res_d3d_device_create_shader_resource_view));
	}

	this->m_forward_shading_vao = NULL;
	{
		D3D11_INPUT_ELEMENT_DESC d3d_input_elements_desc[] =
			{
				{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"NORMAL", 0, DXGI_FORMAT_R8G8B8A8_SNORM, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			};

		HRESULT res_d3d_create_input_layout = d3d_device->CreateInputLayout(d3d_input_elements_desc, sizeof(d3d_input_elements_desc) / sizeof(d3d_input_elements_desc[0]), code_shader_forward_shading_vertex, sizeof(code_shader_forward_shading_vertex), &this->m_forward_shading_vao);
		assert(SUCCEEDED(res_d3d_create_input_layout));
	}

	this->m_forward_shading_vs = NULL;
	{
		HRESULT res_d3d_device_create_vertex_shader = d3d_device->CreateVertexShader(code_shader_forward_shading_vertex, sizeof(code_shader_forward_shading_vertex), NULL, &this->m_forward_shading_vs);
		assert(SUCCEEDED(res_d3d_device_create_vertex_shader));
	}

	this->m_forward_shading_fs = NULL;
	{
		HRESULT res_d3d_device_create_pixel_shader = d3d_device->CreatePixelShader(code_shader_forward_shading_fragment, sizeof(code_shader_forward_shading_fragment), NULL, &this->m_forward_shading_fs);
		assert(SUCCEEDED(res_d3d_device_create_pixel_shader));
	}

	this->m_forward_shading_rs = NULL;
	{
		D3D11_RASTERIZER_DESC d3d_rasterizer_desc;
		d3d_rasterizer_desc.FillMode = D3D11_FILL_SOLID;
		d3d_rasterizer_desc.CullMode = D3D11_CULL_BACK;
		d3d_rasterizer_desc.FrontCounterClockwise = TRUE;
		d3d_rasterizer_desc.DepthBias = 0;
		d3d_rasterizer_desc.SlopeScaledDepthBias = 0.0f;
		d3d_rasterizer_desc.DepthBiasClamp = 0.0f;
		d3d_rasterizer_desc.DepthClipEnable = TRUE;
		d3d_rasterizer_desc.ScissorEnable = FALSE;
		d3d_rasterizer_desc.MultisampleEnable = FALSE;
		d3d_rasterizer_desc.AntialiasedLineEnable = FALSE;
		d3d_rasterizer_desc.MultisampleEnable = FALSE;
		HRESULT res_d3d_device_create_rasterizer_state = d3d_device->CreateRasterizerState(&d3d_rasterizer_desc, &this->m_forward_shading_rs);
		assert(SUCCEEDED(res_d3d_device_create_rasterizer_state));
	}

	this->m_forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer = NULL;
	{
		D3D11_BUFFER_DESC d3d_buffer_desc;
		d3d_buffer_desc.ByteWidth = sizeof(struct forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer);
		d3d_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		d3d_buffer_desc.CPUAccessFlags = 0U;
		d3d_buffer_desc.MiscFlags = 0U;
		d3d_buffer_desc.StructureByteStride = 0U;

		HRESULT res_d3d_device_create_buffer = d3d_device->CreateBuffer(&d3d_buffer_desc, NULL, &this->m_forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer);
		assert(SUCCEEDED(res_d3d_device_create_buffer));
	}

	this->m_forward_shading_pipeline_layout_global_set_object_binding_uniform_buffer = NULL;
	{
		D3D11_BUFFER_DESC d3d_buffer_desc;
		d3d_buffer_desc.ByteWidth = sizeof(struct forward_shading_pipeline_layout_global_set_object_binding_uniform_buffer);
		d3d_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		d3d_buffer_desc.CPUAccessFlags = 0U;
		d3d_buffer_desc.MiscFlags = 0U;
		d3d_buffer_desc.StructureByteStride = 0U;

		HRESULT res_d3d_device_create_buffer = d3d_device->CreateBuffer(&d3d_buffer_desc, NULL, &this->m_forward_shading_pipeline_layout_global_set_object_binding_uniform_buffer);
		assert(SUCCEEDED(res_d3d_device_create_buffer));
	}

	this->m_sphere_vertex_count = 703U;

	this->m_sphere_vb_position = NULL;
	{
		D3D11_BUFFER_DESC d3d_buffer_desc;
		d3d_buffer_desc.ByteWidth = sizeof(sphere_vertex_position);
		d3d_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		d3d_buffer_desc.CPUAccessFlags = 0U;
		d3d_buffer_desc.MiscFlags = 0U;
		d3d_buffer_desc.StructureByteStride = 0U;

		D3D11_SUBRESOURCE_DATA d3d_subresource_data;
		d3d_subresource_data.pSysMem = sphere_vertex_position;
		d3d_subresource_data.SysMemPitch = sizeof(sphere_vertex_position);
		d3d_subresource_data.SysMemSlicePitch = sizeof(sphere_vertex_position);

		HRESULT res_d3d_device_create_buffer = d3d_device->CreateBuffer(&d3d_buffer_desc, &d3d_subresource_data, &this->m_sphere_vb_position);
		assert(SUCCEEDED(res_d3d_device_create_buffer));
	}

	this->m_sphere_vb_varying = NULL;
	{
		int8_t converted_vertex_normal[4U * 703U];

		for (int i_v = 0; i_v < 703U; ++i_v)
		{
			float const *normal_input = &sphere_vertex_normal[3U * i_v];
			int8_t *normal_output = &converted_vertex_normal[4U * i_v];

			normal_output[0] = float_to_snorm(normal_input[0]);
			normal_output[1] = float_to_snorm(normal_input[1]);
			normal_output[2] = float_to_snorm(normal_input[2]);
			// w component
			normal_output[3] = 0;
		}

		D3D11_BUFFER_DESC d3d_buffer_desc;
		d3d_buffer_desc.ByteWidth = sizeof(converted_vertex_normal);
		d3d_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		d3d_buffer_desc.CPUAccessFlags = 0U;
		d3d_buffer_desc.MiscFlags = 0U;
		d3d_buffer_desc.StructureByteStride = 0U;

		D3D11_SUBRESOURCE_DATA d3d_subresource_data;
		d3d_subresource_data.pSysMem = converted_vertex_normal;
		d3d_subresource_data.SysMemPitch = sizeof(converted_vertex_normal);
		d3d_subresource_data.SysMemSlicePitch = sizeof(converted_vertex_normal);

		HRESULT res_d3d_device_create_buffer = d3d_device->CreateBuffer(&d3d_buffer_desc, &d3d_subresource_data, &this->m_sphere_vb_varying);
		assert(SUCCEEDED(res_d3d_device_create_buffer));
	}

	this->m_sphere_index_count = 3672U;

	this->m_sphere_ib = NULL;
	{
		D3D11_BUFFER_DESC d3d_buffer_desc;
		d3d_buffer_desc.ByteWidth = sizeof(sphere_index);
		d3d_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		d3d_buffer_desc.CPUAccessFlags = 0U;
		d3d_buffer_desc.MiscFlags = 0U;
		d3d_buffer_desc.StructureByteStride = 0U;

		D3D11_SUBRESOURCE_DATA d3d_subresource_data;
		d3d_subresource_data.pSysMem = sphere_index;
		d3d_subresource_data.SysMemPitch = sizeof(sphere_index);
		d3d_subresource_data.SysMemSlicePitch = sizeof(sphere_index);

		HRESULT res_d3d_device_create_buffer = d3d_device->CreateBuffer(&d3d_buffer_desc, &d3d_subresource_data, &this->m_sphere_ib);
		assert(SUCCEEDED(res_d3d_device_create_buffer));
	}

	this->m_attachment_backbuffer_rtv = NULL;
	{
		ID3D11Texture2D *attachment_backbuffer = NULL;
		HRESULT res_dxgi_swap_chain_get_buffer = dxgi_swap_chain->GetBuffer(0U, IID_PPV_ARGS(&attachment_backbuffer));
		assert(SUCCEEDED(res_dxgi_swap_chain_get_buffer));

		D3D11_RENDER_TARGET_VIEW_DESC d3d_render_target_view_desc;
		d3d_render_target_view_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		d3d_render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		d3d_render_target_view_desc.Texture2D.MipSlice = 0U;

		HRESULT res_d3d_device_create_render_target_view = d3d_device->CreateRenderTargetView(attachment_backbuffer, &d3d_render_target_view_desc, &this->m_attachment_backbuffer_rtv);
		assert(SUCCEEDED(res_d3d_device_create_render_target_view));
	}

	this->m_attachment_depth = NULL;
	{
		D3D11_TEXTURE2D_DESC d3d_texture2d_desc;
		d3d_texture2d_desc.Width = g_resolution_width;
		d3d_texture2d_desc.Height = g_resolution_width;
		d3d_texture2d_desc.MipLevels = 1U;
		d3d_texture2d_desc.ArraySize = 1U;
		d3d_texture2d_desc.Format = DXGI_FORMAT_R32_TYPELESS;
		d3d_texture2d_desc.SampleDesc.Count = 1U;
		d3d_texture2d_desc.SampleDesc.Quality = 0U;
		d3d_texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
		d3d_texture2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		d3d_texture2d_desc.CPUAccessFlags = 0U;
		d3d_texture2d_desc.MiscFlags = 0U;

		HRESULT res_d3d_device_create_buffer = d3d_device->CreateTexture2D(&d3d_texture2d_desc, NULL, &this->m_attachment_depth);
		assert(SUCCEEDED(res_d3d_device_create_buffer));
	}

	this->m_attachment_depth_dsv = NULL;
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC d3d_depth_stencil_view_desc;
		d3d_depth_stencil_view_desc.Format = DXGI_FORMAT_D32_FLOAT;
		d3d_depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		d3d_depth_stencil_view_desc.Flags = 0U;
		d3d_depth_stencil_view_desc.Texture2D.MipSlice = 0U;

		HRESULT res_d3d_device_create_shader_resource_view = d3d_device->CreateDepthStencilView(this->m_attachment_depth, &d3d_depth_stencil_view_desc, &this->m_attachment_depth_dsv);
		assert(SUCCEEDED(res_d3d_device_create_shader_resource_view));
	}

	g_camera_controller.m_eye_position = DirectX::XMFLOAT3(5.70483208F, 1.27549171F, -1.20246768F);
	g_camera_controller.m_eye_direction = DirectX::XMFLOAT3(-0.934045255F, -0.240197510F, 0.264319360F);
	g_camera_controller.m_up_direction = DirectX::XMFLOAT3(0.0F, 1.0F, 0.0F);

	// sh project equirectangular map
	{
		// uav barrier
		ID3D11UnorderedAccessView *const null_uav = NULL;
		d3d_device_context->CSSetUnorderedAccessViews(0U, 1U, &null_uav, NULL);
		d3d_device_context->CSSetUnorderedAccessViews(0U, 1U, &this->m_environment_map_sh_coefficients_uav, NULL);

		d3d_device_context->CSSetShader(this->m_sh_project_environment_map_clear_cs, NULL, 0U);
		d3d_device_context->CSSetUnorderedAccessViews(0U, 1U, &this->m_environment_map_sh_coefficients_uav, NULL);
		d3d_device_context->Dispatch(1, 1, 1);

		// uav barrier
		d3d_device_context->CSSetUnorderedAccessViews(0U, 1U, &null_uav, NULL);
		d3d_device_context->CSSetUnorderedAccessViews(0U, 1U, &this->m_environment_map_sh_coefficients_uav, NULL);

		DirectX::XMUINT2 environment_equirectangular_map_size(this->m_environment_equirectangular_map_width, this->m_environment_equirectangular_map_height);
		DirectX::XMUINT3 environment_lighting_sh_projection_dispatch_extent = brx_sh_projection_environment_map_dispatch_extent(environment_equirectangular_map_size);

		d3d_device_context->CSSetShader(this->m_sh_project_equirectangular_map_reduction_cs, NULL, 0U);
		d3d_device_context->CSSetShaderResources(0U, 1U, &this->m_environment_equirectangular_map_srv);
		d3d_device_context->CSSetSamplers(0U, 1U, &this->m_clamp_point_sampler);
		d3d_device_context->Dispatch(environment_lighting_sh_projection_dispatch_extent.x, environment_lighting_sh_projection_dispatch_extent.y, environment_lighting_sh_projection_dispatch_extent.z);
	}

	// equirectangular map to octahedral map
	{
		d3d_device_context->CSSetShader(this->m_equirectangular_map_to_octahedral_map_cs, NULL, 0U);
		d3d_device_context->CSSetShaderResources(0U, 1U, &this->m_environment_equirectangular_map_srv);
		d3d_device_context->CSSetSamplers(0U, 1U, &this->m_wrap_anisotropy_sampler);
		d3d_device_context->CSSetUnorderedAccessViews(0U, 1U, &this->m_environment_octahedral_map_uav, NULL);

		d3d_device_context->Dispatch(this->m_environment_octahedral_map_width, this->m_environment_octahedral_map_height, 1);

		// uav barrier
		ID3D11UnorderedAccessView *const null_uav = NULL;
		d3d_device_context->CSSetUnorderedAccessViews(0U, 1U, &null_uav, NULL);
	}

	// sh project octahedral map
	{
		// uav barrier
		ID3D11UnorderedAccessView *const null_uav = NULL;
		d3d_device_context->CSSetUnorderedAccessViews(0U, 1U, &null_uav, NULL);
		d3d_device_context->CSSetUnorderedAccessViews(0U, 1U, &this->m_environment_map_sh_coefficients_uav, NULL);

		d3d_device_context->CSSetShader(this->m_sh_project_environment_map_clear_cs, NULL, 0U);
		d3d_device_context->CSSetUnorderedAccessViews(0U, 1U, &this->m_environment_map_sh_coefficients_uav, NULL);
		d3d_device_context->Dispatch(1, 1, 1);

		// uav barrier
		d3d_device_context->CSSetUnorderedAccessViews(0U, 1U, &null_uav, NULL);
		d3d_device_context->CSSetUnorderedAccessViews(0U, 1U, &this->m_environment_map_sh_coefficients_uav, NULL);

		DirectX::XMUINT2 environment_octahedral_map_size(this->m_environment_octahedral_map_width, this->m_environment_octahedral_map_width);
		DirectX::XMUINT3 environment_lighting_sh_projection_dispatch_extent = brx_sh_projection_environment_map_dispatch_extent(environment_octahedral_map_size);

		d3d_device_context->CSSetShader(this->m_sh_project_octahedral_map_reduction_cs, NULL, 0U);
		d3d_device_context->CSSetShaderResources(0U, 1U, &this->m_environment_octahedral_map_srv);
		d3d_device_context->CSSetSamplers(0U, 1U, &this->m_clamp_point_sampler);
		d3d_device_context->Dispatch(environment_lighting_sh_projection_dispatch_extent.x, environment_lighting_sh_projection_dispatch_extent.y, environment_lighting_sh_projection_dispatch_extent.z);
	}

	// resource barrier
	{
		ID3D11UnorderedAccessView *unordered_access_views[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
		d3d_device_context->CSSetUnorderedAccessViews(0U, 8U, unordered_access_views, NULL);
	}
}

void Demo::Tick(ID3D11Device *d3d_device, ID3D11DeviceContext *d3d_device_context, IDXGISwapChain *dxgi_swap_chain)
{

	// Upload
	{
		struct forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer;
		struct forward_shading_pipeline_layout_global_set_object_binding_uniform_buffer forward_shading_pipeline_layout_global_set_object_binding_uniform_buffer;

		// camera
		DirectX::XMFLOAT3 eye_position = g_camera_controller.m_eye_position;
		DirectX::XMFLOAT3 eye_direction = g_camera_controller.m_eye_direction;
		DirectX::XMFLOAT3 up_direction = g_camera_controller.m_up_direction;

		DirectX::XMMATRIX tmp_view_transform = DirectX::XMMatrixLookToRH(DirectX::XMLoadFloat3(&eye_position), DirectX::XMLoadFloat3(&eye_direction), DirectX::XMLoadFloat3(&up_direction));
		DirectX::XMFLOAT4X4 view_transform;
		DirectX::XMStoreFloat4x4(&view_transform, tmp_view_transform);

		DirectX::XMMATRIX tmp_projection_transform = DirectX::XMMatrixPerspectiveFovRH(0.785F, static_cast<float>(g_resolution_width) / static_cast<float>(g_resolution_height), 0.1F, 100.0F);
		DirectX::XMFLOAT4X4 projection_transform;
		DirectX::XMStoreFloat4x4(&projection_transform, tmp_projection_transform);

		forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer.view_transform = view_transform;
		forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer.projection_transform = projection_transform;
		forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer.eye_position = eye_position;

		// mesh
		DirectX::XMFLOAT4X4 model_transform;
		DirectX::XMStoreFloat4x4(&model_transform, DirectX::XMMatrixIdentity());
		forward_shading_pipeline_layout_global_set_object_binding_uniform_buffer.model_transform = model_transform;

		d3d_device_context->UpdateSubresource(this->m_forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer, 0U, NULL, &forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer, sizeof(struct forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer), sizeof(struct forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer));
		d3d_device_context->UpdateSubresource(this->m_forward_shading_pipeline_layout_global_set_object_binding_uniform_buffer, 0U, NULL, &forward_shading_pipeline_layout_global_set_object_binding_uniform_buffer, sizeof(struct forward_shading_pipeline_layout_global_set_object_binding_uniform_buffer), sizeof(struct forward_shading_pipeline_layout_global_set_object_binding_uniform_buffer));
	}

	// Forward Shading
	{
		d3d_device_context->OMSetRenderTargets(1U, &this->m_attachment_backbuffer_rtv, this->m_attachment_depth_dsv);

		FLOAT color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		d3d_device_context->ClearRenderTargetView(this->m_attachment_backbuffer_rtv, color);

		FLOAT depth = 1.0f;
		d3d_device_context->ClearDepthStencilView(this->m_attachment_depth_dsv, D3D11_CLEAR_DEPTH, depth, 0U);

		D3D11_VIEWPORT viewport = {0.0f, 0.0f, g_resolution_width, g_resolution_height, 0.0f, 1.0f};
		d3d_device_context->RSSetViewports(1U, &viewport);

		d3d_device_context->RSSetState(this->m_forward_shading_rs);

		d3d_device_context->VSSetShader(this->m_forward_shading_vs, NULL, 0U);
		d3d_device_context->PSSetShader(this->m_forward_shading_fs, NULL, 0U);

		d3d_device_context->VSSetConstantBuffers(0U, 1U, &this->m_forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer);
		d3d_device_context->PSSetConstantBuffers(0U, 1U, &this->m_forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer);
		d3d_device_context->VSSetConstantBuffers(1U, 1U, &this->m_forward_shading_pipeline_layout_global_set_object_binding_uniform_buffer);
		d3d_device_context->PSSetConstantBuffers(1U, 1U, &this->m_forward_shading_pipeline_layout_global_set_object_binding_uniform_buffer);
		d3d_device_context->PSSetShaderResources(0U, 1U, &this->m_environment_map_sh_coefficients_srv);
		d3d_device_context->PSSetShaderResources(1U, 1U, &this->m_hdr_norms_lut_srv);
		d3d_device_context->PSSetShaderResources(2U, 1U, &this->m_sh_transfer_functions_lut_srv);

		d3d_device_context->PSSetSamplers(0U, 1U, &this->m_clamp_linear_sampler);

		d3d_device_context->IASetInputLayout(this->m_forward_shading_vao);

		// Draw Sphere
		ID3D11Buffer *vertex_buffers[2] = {this->m_sphere_vb_position, this->m_sphere_vb_varying};
		UINT strides[2] = {sizeof(float) * 3, sizeof(int8_t) * 4};
		UINT offsets[2] = {0U, 0U};
		d3d_device_context->IASetVertexBuffers(0U, 2U, vertex_buffers, strides, offsets);
		d3d_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		d3d_device_context->IASetIndexBuffer(this->m_sphere_ib, DXGI_FORMAT_R16_UINT, 0U);
		d3d_device_context->DrawIndexedInstanced(this->m_sphere_index_count, 1U, 0U, 0U, 0U);
	}

	// resource barrier
	{
		ID3D11ShaderResourceView *shader_resource_views[3] = {NULL, NULL, NULL};
		d3d_device_context->PSSetShaderResources(0U, 3U, shader_resource_views);
	}

	HRESULT res_dxgi_swap_chain_present = dxgi_swap_chain->Present(1U, 0U);
	assert(SUCCEEDED(res_dxgi_swap_chain_present));
}

static inline int8_t float_to_snorm(float unpacked_input)
{
	// d3dx_dxgiformatconvert.inl
	// D3DX_FLOAT4_to_R8G8B8A8_SNORM

	// UE: [FPackedNormal](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Source/Runtime/RenderCore/Public/PackedNormal.h#L98)

	float saturate_signed_float = std::min(std::max(unpacked_input, -1.0F), 1.0F);
	float float_to_int = saturate_signed_float * 127.0F + (saturate_signed_float >= 0 ? 0.5F : -0.5F);
	float truncate_float = float_to_int >= 0 ? std::floor(float_to_int) : std::ceil(float_to_int);
	return ((int8_t)truncate_float);
}