VertexAttribs "StandardTriMesh" {
    Output "vec3" "Position";
    Output "vec3" "Normal";
    Output "vec4" "Tangent";
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

ParameterBlock "PerPrimitive" : Set(2) {
    Output "uniform" "PerPrimitiveConstants" [[
        mat4 ModelMat;
        mat4 NormalToWorld;
    ]] : Stages "V";
};

function StaticMeshVS()
    Input "vec3" "Position";
    Input "vec3" "Normal";
    Input "vec2" "TexCoord0";
    Input "uniform" "GlobalConstants";
    Input "uniform" "PerPrimitiveConstants";
    Output "vec3" "iPosition";
    Output "vec3" "iNormal";
    Output "vec4" "iTangent";
    Output "vec2" "iTexCoord0";
    Code [[
        vec4 pos = ModelMat * vec4(Position, 1);
        gl_Position = ProjMat * ViewMat * pos;
        iPosition = (pos / pos.w).xyz;
        iTexCoord0 = TexCoord0;
        iNormal = normalize(mat3(ViewMat) * mat3(ModelMat) * Normal);
    ]];
end

function StaticMeshZOnlyVS()
    Input "vec3" "Position";
    Input "vec2" "TexCoord0";
    Input "uniform" "GlobalConstants";
    Input "uniform" "PerPrimitiveConstants";
    Output "vec2" "iTexCoord0";
    Code [[
        gl_Position = ProjMat * ViewMat * ModelMat * vec4(Position, 1);
        iTexCoord0 = TexCoord0;
    ]];
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

ParameterBlock "BasicMaterialParams" : Set(1) {
    Output "uniform" "MaterialConstants" [[
        vec4 BaseColorFactor;
        vec4 MetallicRoughness;
        bool UseTextures;
    ]] : Stages "P";
    Output "texture2D" "BaseColorTex" : Stages "P";
    Output "texture2D" "MetallicRoughnessTex" : Stages "P";
};

function BasicMaterial()
    Input "vec2" "iTexCoord0";
    Input "uniform" "MaterialConstants";
    Input "texture2D" "BaseColorTex";
    Input "texture2D" "MetallicRoughnessTex";
    Output "vec4" "BaseColor";
    Output "float" "Metallic";
    Output "float" "Roughness";

    Code [[
        if (!UseTextures)
        {
            BaseColor = BaseColorFactor;
            Metallic = MetallicRoughness.g;
            Roughness = MetallicRoughness.b;
        }
        else
        {
            BaseColor = texture(sampler2D(BaseColorTex, GlobalLinearSampler), iTexCoord0) * BaseColorFactor;

            if (BaseColor.a < 0.05) discard;

            vec4 mr = texture(sampler2D(MetallicRoughnessTex, GlobalLinearSampler), iTexCoord0);
            Metallic = mr.g * MetallicRoughness.g;
            Roughness = mr.b * MetallicRoughness.b;
        }
    ]]
end

function BasicZOnlyMaterial()
    Input "vec2" "iTexCoord0";
    Input "uniform" "MaterialConstants";
    Input "texture2D" "BaseColorTex";

    Code [[
        if (UseTextures) {
            float alpha = texture(sampler2D(BaseColorTex, GlobalLinearSampler), iTexCoord0).a * BaseColorFactor.a;
            if (alpha < 0.05) {
                discard;
            }
        }
    ]]
end

function GBufferPS()
    Input "vec4" "BaseColor";
    Input "vec3" "iNormal";
    Output "vec4" "Target0";
    Output "vec4" "Target1";
    Code [[
        Target0 = vec4(BaseColor.rgb, 1.0);
        Target1 = vec4(fma(iNormal, vec3(0.5), vec3(0.5)), 0.0);
    ]]
end

function NormalVisPS()
    Input "vec3" "iNormal";
    Output "vec4" "Target0";
    Code [[
        Target0 = iWorldNormal / 2 + 0.5;
    ]]
end

DepthStencil "DefaultDepthStencil" {
};

Blend "DefaultBlend" {
};
