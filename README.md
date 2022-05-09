# Environment Lighting  

The name **environment lighting** is from "10.2 Environment Lighting" of [Real-Time Rendering Fourth Edition](https://www.realtimerendering.com/), ["12.6 Infinite Area Lights"](https://pbr-book.org/3ed-2018/Light_Sources/Infinite_Area_Lights) of [PBR Book V3](https://pbr-book.org/3ed-2018/contents), and ["12.5 Infinite Area Lights"](https://pbr-book.org/4ed/Light_Sources/Infinite_Area_Lights) of [PBR Book V4](https://pbr-book.org/4ed/contents), while the **environment lighting** is called [HDRI Sky](https://docs.unity3d.com/Packages/com.unity.render-pipelines.high-definition@10.10/manual/Override-HDRI-Sky.html) in Unity3D and [Sky Light](https://dev.epicgames.com/documentation/en-us/unreal-engine/sky-light?application_version=4.27) in UnrealEngine.  

- [x] brx_sh_project_environment_map_reduction: the Octahedral/Equirectangular Map GPU implementation of [DirectX::SHProjectCubeMap](https://github.com/microsoft/DirectXMath/blob/jul2018b/SHMath/DirectXSHD3D11.cpp#L169)  
- [x] brx_Image_based_lighting: the environment map space (+Z Up; +X Front) may be different from the scene, the normal (N) should be properly transformed before use  
