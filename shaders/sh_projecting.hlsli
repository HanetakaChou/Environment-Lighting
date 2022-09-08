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

#ifndef _SH_PROJECTING_HLSLI_
#define _SH_PROJECTING_HLSLI_ 1

struct SH_L2
{
	// 0 -> (0,  0)
	// 1 -> (1, -1)
	// 2 -> (1,  0)
	// 3 -> (1,  1)
	// 4 -> (2, -2)
	// 5 -> (2, -1)
	// 6 -> (2,  0)
	// 7 -> (2,  1)
	// 8 -> (2,  2)
	float p[9];
};

struct SH_L2_RGB
{
	struct SH_L2 r;
	struct SH_L2 g;
	struct SH_L2 b;
};

// NOTE: omega should be normalized before using the "polynomial form"
struct SH_L2 evaluate_sh_l2_basis(float3 direction);

struct SH_L2 evaluate_sh_l2_zero();

struct SH_L2 evaluate_sh_l2_scale(struct SH_L2 x, float scale);

struct SH_L2 evaluate_sh_l2_add(struct SH_L2 x, struct SH_L2 y);

struct SH_L2_RGB evaluate_sh_l2_rgb_zero();

struct SH_L2_RGB evaluate_sh_l2_scale_rgb(struct SH_L2 x, float3 scale);

struct SH_L2_RGB evaluate_sh_l2_rgb_scale(struct SH_L2_RGB x, float scale);

struct SH_L2_RGB evaluate_sh_l2_rgb_add(struct SH_L2_RGB x, struct SH_L2_RGB y);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//    IMPLEMENTATION
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
struct SH_L2 evaluate_sh_l2_basis(float3 direction)
{
	// "Appendix A2" of [Sloan 2008]: polynomial form of SH basis
	// UE4: [SHBasisFunction3](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/SHCommon.ush#L226)
	// DirectXMath: [sh_eval_basis_2](https://github.com/microsoft/DirectXMath/blob/jul2018b/SHMath/DirectXSH.cpp#L132)

	struct SH_L2 upsilon;

	upsilon.p[0] = 0.282094791773878140;

	upsilon.p[1] = -0.488602511902919920 * direction.y;
	upsilon.p[2] = 0.488602511902919920 * direction.z;
	upsilon.p[3] = -0.488602511902919920 * direction.x;

	upsilon.p[4] = 1.092548430592079200 * direction.x * direction.y;
	upsilon.p[5] = -1.092548430592079200 * direction.y * direction.z;
	upsilon.p[6] = 0.946174695757560080 * direction.z * direction.z - 0.315391565252520050;
	upsilon.p[7] = -1.092548430592079200 * direction.x * direction.z;
	upsilon.p[8] = 0.546274215296039590 * (direction.x * direction.x - direction.y * direction.y);

	return upsilon;
}

struct SH_L2 evaluate_sh_l2_zero()
{
	struct SH_L2 x;

	x.p[0] = 0.0;
	x.p[1] = 0.0;
	x.p[2] = 0.0;
	x.p[3] = 0.0;
	x.p[4] = 0.0;
	x.p[5] = 0.0;
	x.p[6] = 0.0;
	x.p[7] = 0.0;
	x.p[8] = 0.0;

	return x;
}

struct SH_L2 evaluate_sh_l2_scale(struct SH_L2 x, float scale)
{
	struct SH_L2 y;

	y.p[0] = x.p[0] * scale;
	y.p[1] = x.p[1] * scale;
	y.p[2] = x.p[2] * scale;
	y.p[3] = x.p[3] * scale;
	y.p[4] = x.p[4] * scale;
	y.p[5] = x.p[5] * scale;
	y.p[6] = x.p[6] * scale;
	y.p[7] = x.p[7] * scale;
	y.p[8] = x.p[8] * scale;

	return y;
}

struct SH_L2 evaluate_sh_l2_add(struct SH_L2 x, struct SH_L2 y)
{
	struct SH_L2 z;

	z.p[0] = x.p[0] + y.p[0];
	z.p[1] = x.p[1] + y.p[1];
	z.p[2] = x.p[2] + y.p[2];
	z.p[3] = x.p[3] + y.p[3];
	z.p[4] = x.p[4] + y.p[4];
	z.p[5] = x.p[5] + y.p[5];
	z.p[6] = x.p[6] + y.p[6];
	z.p[7] = x.p[7] + y.p[7];
	z.p[8] = x.p[8] + y.p[8];

	return z;
}

struct SH_L2_RGB evaluate_sh_l2_rgb_zero()
{
	struct SH_L2_RGB x;

	x.r = evaluate_sh_l2_zero();
	x.g = evaluate_sh_l2_zero();
	x.b = evaluate_sh_l2_zero();

	return x;
}

struct SH_L2_RGB evaluate_sh_l2_scale_rgb(struct SH_L2 x, float3 scale)
{
	struct SH_L2_RGB y;

	y.r = evaluate_sh_l2_scale(x, scale.r);
	y.g = evaluate_sh_l2_scale(x, scale.g);
	y.b = evaluate_sh_l2_scale(x, scale.b);

	return y;
}

struct SH_L2_RGB evaluate_sh_l2_rgb_scale(struct SH_L2_RGB x, float scale)
{
	struct SH_L2_RGB y;

	y.r = evaluate_sh_l2_scale(x.r, scale);
	y.g = evaluate_sh_l2_scale(x.g, scale);
	y.b = evaluate_sh_l2_scale(x.b, scale);

	return y;
}

struct SH_L2_RGB evaluate_sh_l2_rgb_add(struct SH_L2_RGB x, struct SH_L2_RGB y)
{
	struct SH_L2_RGB z;

	z.r = evaluate_sh_l2_add(x.r, y.r);
	z.g = evaluate_sh_l2_add(x.g, y.g);
	z.b = evaluate_sh_l2_add(x.b, y.b);

	return z;
}

#endif