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

#ifndef _BRDF_HLSLI_
#define _BRDF_HLSLI_ 1

#include "math_consts.hlsli"

float3 Diffuse_Lambert(float3 diffuse_color)
{
	return diffuse_color * (1.0 / PI);
}

float D_TR(float alpha, float NdotH)
{
	// Trowbridge-Reitz

	// Equation 9.41 of Real-Time Rendering Fourth Edition: "Although ��Trowbridge-Reitz distribution�� is technically the correct name"
	// Equation 8.11 of PBR Book: https://pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models#MicrofacetDistributionFunctions
	float alpha2 = alpha * alpha;
	float denominator = 1.0 + NdotH * (NdotH * alpha2 - NdotH);
	return (1.0 / PI) * (alpha2 / (denominator * denominator));
}

float V_HC_TR(float alpha, float NdotV, float NdotL)
{
	// Height-Correlated Trowbridge-Reitz

	// Lambda:
	// Equation 8.13 of PBR Book: https://pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models#MaskingandShadowing

	// Lambda for Trowbridge-Reitz:
	// Equation 9.42 of Real-Time Rendering Fourth Edition
	// Figure 8.18 of PBR Book: https://pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models#MaskingandShadowing
	// ��(V) = 0.5*(-1.0 + (1.0/NoV)*sqrt(alpha^2 + (1.0 - alpha^2)*NoV^2))
	// ��(L) = 0.5*(-1.0 + (1.0/NoL)*sqrt(alpha^2 + (1.0 - alpha^2)*NoL^2))

	// G2
	// Equation 9.31 of Real-Time Rendering Fourth Edition
	// PBR Book / 8.4.3 Masking and Shadowing: "A more accurate model can be derived assuming that microfacet visibility is more likely the higher up a given point on a microface"
	// G2 = 1.0/(1.0 + ��(V) + ��(L)) = (2.0*NoV*NoL)/(NoL*sqrt(alpha^2 + (1.0 - alpha^2)*NoV^2) + NoV*sqrt(alpha^2 + (1.0 - alpha^2)*NoL^2))

	// V = G2/(4.0*NoV*NoL) = 0.5/(NoL*sqrt(alpha^2 + (1.0 - alpha^2)*NoV^2) + NoV*sqrt(alpha^2 + (1.0 - alpha^2)*NoL^2))

	// float alpha2 = alpha * alpha;
	// float term_v = NdotL * sqrt(alpha2 + (1.0 - alpha2) * NdotV * NdotV);
	// float term_l = NdotV * sqrt(alpha2 + (1.0 - alpha2) * NdotL * NdotL);
	// UE: [Vis_SmithJointApprox](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L380)
	float term_v = NdotL * (alpha + (1.0 - alpha) * NdotV);
	float term_l = NdotV * (alpha + (1.0 - alpha) * NdotL);
	return (0.5 / (term_v + term_l));
}

#endif