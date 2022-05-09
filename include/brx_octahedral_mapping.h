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

#ifndef _BRX_OCTAHEDRAL_MAPPING_H_
#define _BRX_OCTAHEDRAL_MAPPING_H_ 1

// [Engelhardt 2008] Thomas Engelhardt, Carsten Dachsbacher. "Octahedron Environment Maps." VMV 2008.
// [Cigolle 2014] Zina Cigolle, Sam Donow, Daniel Evangelakos, Michael Mara, Morgan McGuire, Quirin Meyer. "A Survey of Efficient Representations for Independent Unit Vectors." JCGT 2014.

// Real-Time Rendering Fourth Edition: 10.4.4 Other Projections
// Real-Time Rendering Fourth Edition: 16.6 Compression and Precision
// [PBR Book V4: 12.5.2 Image Infinite Lights](https://www.pbr-book.org/4ed/Light_Sources/Infinite_Area_Lights#ImageInfiniteLights)
// [PBRT-V: ImageInfiniteLight::Le](https://github.com/mmp/pbrt-v4/blob/master/src/pbrt/lights.h#L574)

static inline DirectX::XMFLOAT2 brx_octahedral_map(DirectX::XMFLOAT3 const &position_sphere_surface)
{
    // NOTE: positions on the sphere surface should have already been normalized
    assert(std::abs(((position_sphere_surface.x * position_sphere_surface.x) + (position_sphere_surface.y * position_sphere_surface.y) + (position_sphere_surface.z * position_sphere_surface.z)) - 1.0F) < 0.001F);

    float const manhattan_norm = std::abs(position_sphere_surface.x) + std::abs(position_sphere_surface.y) + std::abs(position_sphere_surface.z);

    DirectX::XMFLOAT3 position_octahedron_surface;
    DirectX::XMStoreFloat3(&position_octahedron_surface, DirectX::XMVectorScale(DirectX::XMLoadFloat3(&position_sphere_surface), 1.0F / manhattan_norm));

    DirectX::XMFLOAT2 const position_ndc_space = (position_octahedron_surface.z > 0.0F) ? DirectX::XMFLOAT2(position_octahedron_surface.x, position_octahedron_surface.y) : DirectX::XMFLOAT2((1.0F - std::abs(position_octahedron_surface.y)) * ((position_octahedron_surface.x >= 0.0F) ? 1.0F : -1.0F), (1.0F - std::abs(position_octahedron_surface.x)) * ((position_octahedron_surface.y >= 0.0F) ? 1.0F : -1.0F));
    return position_ndc_space;
}

static inline DirectX::XMFLOAT3 brx_octahedral_unmap(DirectX::XMFLOAT2 const &position_ndc_space)
{
    float const position_octahedron_surface_z = 1.0F - std::abs(position_ndc_space.x) - std::abs(position_ndc_space.y);

    DirectX::XMFLOAT2 const position_octahedron_surface_xy = (position_octahedron_surface_z >= 0.0F) ? position_ndc_space : DirectX::XMFLOAT2((1.0F - std::abs(position_ndc_space.y)) * ((position_ndc_space.x >= 0.0F) ? 1.0F : -1.0F), (1.0F - std::abs(position_ndc_space.x)) * ((position_ndc_space.y >= 0.0F) ? 1.0F : -1.0F));

    DirectX::XMFLOAT3 const position_octahedron_surface = DirectX::XMFLOAT3(position_octahedron_surface_xy.x, position_octahedron_surface_xy.y, position_octahedron_surface_z);

    DirectX::XMFLOAT3 position_sphere_surface;
    DirectX::XMStoreFloat3(&position_sphere_surface, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&position_octahedron_surface)));
    return position_sphere_surface;
}

#endif
