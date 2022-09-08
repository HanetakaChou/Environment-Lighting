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
	ID3D11SamplerState *m_clamp_point_sampler;
	ID3D11SamplerState *m_clamp_linear_sampler;

	ID3D11Texture2D *m_distant_radiance_distribution;
	ID3D11ShaderResourceView *m_distant_radiance_distribution_srv;
	
	ID3D11ComputeShader *m_integrate_form_factor_normal_distribution_cs;
	ID3D11Buffer *m_form_factor_normal_distribution;
	ID3D11UnorderedAccessView *m_form_factor_normal_distribution_uav;
	ID3D11ShaderResourceView *m_form_factor_normal_distribution_srv;

	ID3D11ComputeShader *m_integrate_ld_normal_distribution_cs;
	ID3D11Buffer *m_integrate_ld_normal_distribution_pipeline_layout_global_set_distribution_binding_uniform_buffer;
	uint32_t m_ld_normal_distribution_width;
	uint32_t m_ld_normal_distribution_height;
	uint32_t m_ld_normal_distribution_mip_level_count;
	ID3D11Texture2D *m_ld_normal_distribution;
	ID3D11UnorderedAccessView *m_ld_normal_distribution_uav[MAX_CUBE_MIP_LEVEL_COUNT];
	ID3D11ShaderResourceView *m_ld_normal_distribution_srv;

	ID3D11ComputeShader *m_integrate_hemispherical_directional_reflectance_cs;
	uint32_t m_hemispherical_directional_reflectance_lut_width;
	uint32_t m_hemispherical_directional_reflectance_lut_height;
	uint32_t m_hemispherical_directional_reflectance_lut_array_layer_count;
	ID3D11Texture2D *m_hemispherical_directional_reflectance_lut;
	ID3D11UnorderedAccessView *m_hemispherical_directional_reflectance_lut_uav;
	ID3D11ShaderResourceView *m_hemispherical_directional_reflectance_lut_srv;

	ID3D11InputLayout *m_forward_shading_vao;
	ID3D11VertexShader *m_forward_shading_vs;
	ID3D11PixelShader *m_forward_shading_fs;
	ID3D11RasterizerState *m_forward_shading_rs;
	ID3D11Buffer *m_forward_shading_pipeline_layout_global_set_frame_binding_uniform_buffer;
	ID3D11Buffer* m_forward_shading_pipeline_layout_global_set_object_binding_uniform_buffer;

	uint32_t m_sphere_vertex_count;
	ID3D11Buffer* m_sphere_vb_position;
	ID3D11Buffer* m_sphere_vb_varying;
	uint32_t m_sphere_index_count;
	ID3D11Buffer* m_sphere_ib;

	ID3D11RenderTargetView* m_attachment_backbuffer_rtv;
	ID3D11Texture2D *m_attachment_depth;
	ID3D11DepthStencilView *m_attachment_depth_dsv;

public:
	void Init(ID3D11Device *d3d11_device, ID3D11DeviceContext* d3d11_device_context, IDXGISwapChain* dxgi_swap_chain);
	void Tick(ID3D11Device* d3d11_device, ID3D11DeviceContext *d3d11_device_context, IDXGISwapChain* dxgi_swap_chain);
};

#endif