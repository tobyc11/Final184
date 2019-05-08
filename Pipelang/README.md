# Pipelang

A DSL for efficient graphics pipeline meta-programming.

The idea is that you author a shader library comprised of many indivdual shader stages. A stage in Pipelang does not necessarily correspond to a hardware stage, for example, you can break down the pixel shader into a material stage and a lighting stage. And at runtime, the application specifies a pipeline by supplying a list of stages.

Pipelang distinguishes between 3 kinds of stages: `Inteface`, `Programmable`, and `Fixed`. The interface stages contitute the API contract between the host application and the shading pipeline; furthermote, each `ParameterBlock` stage corresponds to a descriptor layout in Vulkan. Hence, the programmable stages you write must be compatible with the interface stages you define. The fixed function stages act as dividers between the programmable stages.

### Shader Library
```
VertexAttribs "StandardTriMesh" {
    Output "vec3" "Position";
    Output "vec3" "Normal";
    Output "vec3" "Tangent";
    Output "vec2" "TexCoord0";
};

ParameterBlock "EngineCommon" : Set(0) {
    Output "sampler" "GlobalNiceSampler" : Stages "P";
    Output "sampler" "GlobalLinearSampler" : Stages "P";
    Output "sampler" "GlobalNearestSampler" : Stages "P";
    Output "uniform" "GlobalConstants" [[
        vec4 CameraPos;
        mat4 ViewMat;
        mat4 ProjMat;
        mat4 InvProj;
    ]] : Stages "VDHGP";
};

ParameterBlock "PerPrimitive" : Set(3) {
    Output "uniform" "PerPrimitiveConstants" [[
        mat4 ModelMat;
        mat4 NormalToWorld;
    ]] : Stages "V";
};

function StaticMeshVS()
    Input "vec3" "Position";
    Input "uniform" "GlobalConstants";
    Input "uniform" "PerPrimitiveConstants";
    Output "vec3" "iWorldPosition";
    Output "vec3" "iWorldNormal";
    Code [[
        vec4 world = ModelMat * vec4(Position, 1);
        gl_Position = ProjMat * ViewMat * world;
        iWorldPosition = world.xyz / world.w;
    ]];

    if Normal then
        Input "vec3" "Normal";
        Code [[
            iWorldNormal = mat3(ModelMat) * Normal;
        ]];
    else
        Code [[
            iWorldNormal = vec3(0, 0, 1);
        ]];
    end
end

Rasterizer "DefaultRasterizer" {
    PolygonMode = "Fill";
    CullMode = "Back";
    FrontFaceCCW = true;
    DepthBiasEnable = false;
    DepthBiasConstantFactor = 0;
    DepthBiasClamp = 0;
    DepthBiasSlopeFactor = 0;
    DepthClampEnable = false;
};

function NormalVisPS()
    Input "vec3" "iWorldNormal";
    Output "vec3" "Target0";
    Code [[
        Target0 = iWorldNormal / 2 + 0.5;
    ]]
end

DepthStencil "DefaultDepthStencil" {
};

Blend "DefaultBlend" {
};
```

### Runtime Interface
Setup
```
// Inside some global function
auto& param = Library.GetParameterBlock("EngineCommon");
auto dsEngine = param.CreateDescriptorSet();
param.BindSampler(sampler0, "GlobalNiceSampler");
...

// Inside the material class
auto& param = Library.GetParameterBlock("BasicMaterialParams");
auto dsMat = param.CreateDescriptorSet();
param.BindBuffer(buf0, 0, 32, "MaterialConstants");
param.BindImage(albedo, "AlbedoImage");
```

Draw
```
// The pipeline binding can change as long as the parameter blocks stay the same
auto pipeline = Library.GetPipeline("EngineCommon", "StaticMeshVS", "DefaultRasterizer", "BasicMaterialParams", "BasicMaterial", "GBufferPS", renderPass, subpass);
ctx.BindPipeline(*pipeline);
ctx.BindDescriptorSet(0, dsEngine);

auto& param = Library.GetParameterBlock("PerPrimitive");
auto dsPrim = param.CreateDescriptorSet();

for (material) {
	ctx.BindDescriptorSet(1, dsMat);
	for (primitive) {
		ctx.BindPipeline(*Library.GetPipeline("EngineCommon", prim->MeshKind(), "DefaultRasterizer", "BasicMaterialParams", "BasicMaterial", "GBufferPS", renderPass, subpass));
		dsPrim.BindConstants(...);
		ctx.BindDescriptorSet(2, dsPrim);
	}
}
```
