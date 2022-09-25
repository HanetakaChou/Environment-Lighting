#ifndef _UNIFIED_INTEGRATE_PIPELINE_LAYOUT_H_
#define _UNIFIED_INTEGRATE_PIPELINE_LAYOUT_H_ 1

#if defined(__STDC__) || defined(__cplusplus)


#elif defined(GL_SPIRV) || defined(VULKAN)

layout(set = 0, binding = 0) uniform highp sampler clamp_point_sampler;

layout(set = 0, binding = 1) uniform highp textureCube Distant_Radiance_Distribution;

layout(set = 1, binding = 0) writeonly buffer RWByteAddressBuffer_Packed_SH_L2_RGB
{
	// x: P(1, 1)
	// y: P(1,-1)
	// z: P(1, 0)
	// w: P(0, 0) + part of P(2, 0)
	vec4 cAr;
	vec4 cAg;
	vec4 cAb;

	// x: P(2,-2)
	// y: P(2,-1)
	// z: part of P(2, 0)
	// w: P(2, 1)
	vec4 cBr;
	vec4 cBg;
	vec4 cBb;

	// r: P(2, 2)
	// g: P(2, 2)
	// b: P(2, 2)
	vec3 cC;
} Form_Factor_Normal_Distribution;

#else
#error Unknown Compiler
#endif

#endif