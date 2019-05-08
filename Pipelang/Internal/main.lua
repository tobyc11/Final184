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
