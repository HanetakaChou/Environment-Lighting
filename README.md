# Spherical Harmonic  

- [x] brx_spherical_harmonic_projection_environment_map_reduce: the Octahedral/Equirectangular Map GPU implementation of [DirectX::SHProjectCubeMap](https://github.com/microsoft/DirectXMath/blob/jul2018b/SHMath/DirectXSHD3D11.cpp#L169)  
- [x] brx_spherical_harmonic_diffuse_radiance: the environment map space (+Z Up; +X Front) may be different from the world space, the normal (N) should be properly transformed before use  
- [x] brx_spherical_harmonic_specular_radiance: the environment map space (+Z Up; +X Front) may be different from the world space, the outgoing direction (V) and the normal (N) should be properly transformed before use  
