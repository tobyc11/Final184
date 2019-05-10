--[[
We have different kinds of stages.
Interface stages (VertexAttribs and ParameterBlock) are immutable and can only have outputs.
Programmable stages are functions run on the fly.
Fixed function stages set pipeline states and possibly delimits the other stages
]]--

parser = {}

local stage_class = { interface = "interface", programmable = "programmable", fixed = "fixed" }
local stage_subclass = { vertex_attribs = "vertex_attribs", VertexAttribs = "vertex_attribs",
                         parameter_block = "parameter_block", ParameterBlock = "parameter_block",
                         rasterizer = "rasterizer", Rasterizer = "rasterizer",
                         depthstencil = "depthstencil", DepthStencil = "depthstencil",
                         blend = "blend", Blend = "blend", GeometryShader = "geometry_shader" }
local shader_stage = { undefined = "undefined", vertex = "vertex", domain = "domain", hull = "hull",
                       geometry = "geometry", pixel = "pixel", compute = "compute" }

parser.stage_class = stage_class
parser.stage_subclass = stage_subclass
parser.shader_stage = shader_stage

local all_stages = {}
local interface_stages = {}
local programmable_stages = {}
local fixed_stages = {}

parser.all_stages = all_stages
parser.interface_stages = interface_stages
parser.programmable_stages = programmable_stages
parser.fixed_stages = fixed_stages

local function add_stage(k, v)
    local result = {}
    result.name = k
    all_stages[k] = result
    
    result.stage = shader_stage.undefined
    if type(v) == "function" then
        -- Do not preprocess functions
        programmable_stages[k] = result
        result.class = stage_class.programmable
        result.fn = v
    else
        -- Has to be a table
        if v.type == "VertexAttribs" or v.type == "ParameterBlock" then
            local locKey = "binding"
            if v.type == "VertexAttribs" then
                locKey = "location"
            end

            interface_stages[k] = result
            result.class = stage_class.interface
            result.subclass = stage_subclass[v.type]
            result.inputs = {}
            result.outputs = {}

            result.set = rawget(v, "Set")

            local locationCounter = 0
            local usedLocations = {}
            local function nextLoc()
                while usedLocations[locationCounter] do
                    locationCounter = locationCounter + 1
                end
                usedLocations[locationCounter] = true
                return locationCounter
            end

            for _, output in ipairs(v.a0) do
                assert(output.type == "Output")
                result.outputs[output.a1] = {}
                result.outputs[output.a1].parent = k
                result.outputs[output.a1].type = output.a0
                result.outputs[output.a1].used = false
                if output.a0 == "uniform" then
                    result.outputs[output.a1].code = output.a2
                end
                result.outputs[output.a1][locKey] = nextLoc()
                result.outputs[output.a1].stages = rawget(output, "Stages")
                if result.outputs[output.a1].stages == nil then
                    result.outputs[output.a1].stages = "VDHGP"
                end
                result.outputs[output.a1].set = rawget(v, "Set")
                if not result.outputs[output.a1].set then
                    result.outputs[output.a1].set = 0
                end
				result.outputs[output.a1].format = rawget(output, "Format")
                result.outputs[output.a1].count = 1
            end
        else
            fixed_stages[k] = result
            result.class = stage_class.fixed
            result.subclass = stage_subclass[v.type]
            result.inputs = {}
            result.outputs = {}
            result.states = v.a0
        end
    end
end

for k, v in pairs(sandbox) do
    add_stage(k, v)
end

function parser.add_all_interface_stages()
    for name, block in pairs(interface_stages) do
        if block.subclass == stage_subclass.parameter_block then
            local pb = rt.CParameterBlock()
            pb:SetSetIndex(block.set)
            for bindingName, output in pairs(block.outputs) do
                pb:AddBinding(bindingName, output.binding, output.type, output.count, output.stages)
            end
            library:AddParameterBlock(name, pb)
        elseif block.subclass == stage_subclass.vertex_attribs then
            local va = rt.CVertexAttribs()
            for attribName, output in pairs(block.outputs) do
                va:AddAttribute(attribName, output.location)
            end
            library:AddVertexAttribs(name, va)
        end
    end
end
