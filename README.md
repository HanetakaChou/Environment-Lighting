The name **environment lighting** is from "10.2 Environment Lighting" of [Real-Time Rendering Fourth Edition](https://www.realtimerendering.com/). The most important difference between environment light and global illumination is that the shading algorithm of the environment light is independent of the other positions on the surface except the shading position.  

- integrate_form_factor_normal_distribution.hlsl: precompute form factor  
- integrate_ld_normal_distribution.hlsl: precompute DFG term  
- integrate_hemispherical_directional_reflectance.hlsl: precompute LD term  
- environment_lighting.hlsli: compute environment lighting on-the-fly  
