codegen = {}

local function setfenv(fn, env)
    local i = 1
    while true do
        local name = debug.getupvalue(fn, i)
        if name == "_ENV" then
            debug.upvaluejoin(fn, i, (function()
                return env
            end), 1)
            break
        elseif not name then
            break
        end
        i = i + 1
    end
    return fn
end

function codegen.annotate_parse_tree(stage_list)
    local symtab = {}
    local curr_stage = parser.shader_stage.vertex
    local programmable_output_counter = 0

    for _, stage_name in ipairs(stage_list) do
        local stage = parser.all_stages[stage_name]
        stage.stage = curr_stage
        if stage.class == "interface" then
            for name, var in pairs(stage.outputs) do
                symtab[name] = var
                var.used = false
            end
        elseif stage.class == "fixed" then
            if stage.subclass == "rasterizer" then
                curr_stage = parser.shader_stage.pixel
            end
        elseif stage.class == "programmable" then
            stage.inputs = {}
            stage.outputs = {}
            stage.code = ""
            local PROG_MT = {}
            PROG_MT.__index = function (t, k)
                if symtab[k] then
                    return symtab[k]
                end

                if k == "Input" then
                    return function (type)
                        return function (name)
                            if symtab[name] == nil then
                                print("Error: input not found", name)
                            end
                            stage.inputs[name] = symtab[name]
                            symtab[name].used = true
                            symtab[name].used_export = false
                            if curr_stage ~= parser.all_stages[symtab[name].parent].stage then
                                symtab[name].used_export = true
                            end
                        end
                    end
                end

                if k == "Output" then
                    return function (type)
                        return function (name)
                            stage.outputs[name] = {
                                type = type,
                                parent = stage.name,
                                count = 1,
                                location = programmable_output_counter
                            }
                            programmable_output_counter = programmable_output_counter + 1
                        end
                    end
                end

                if k == "Code" then
                    return function (code)
                        stage.code = stage.code .. code
                    end
                end

                return nil
            end
            setfenv(stage.fn, setmetatable({}, PROG_MT))()

            for name, var in pairs(stage.outputs) do
                symtab[name] = var
                var.used = false
            end
        else
            print("Error: stage unknown", stage_name)
        end
    end
end

function codegen.glsl_gen(stage_list, curr_stage)
    local glsl_src = { header = "", main = "" }

    function glsl_src:emit_semicolon()
        self.header = self.header .. ";\n";
    end

    function glsl_src:emit_block(content)
        self.header = self.header .. "{\n" .. content .. "};\n";
    end

    function glsl_src:emit_stmts_main(content)
        self.main = self.main .. content .. "\n";
    end

    function glsl_src:emit_global_decl(name, obj, input)
        if obj.binding then
            self.header = self.header ..
                string.format("layout(set=%d, binding=%d) uniform ", obj.set, obj.binding)
        elseif obj.used_export and not input then
            self.header = self.header ..
                string.format("layout(location=%d) out ", obj.location)
        elseif obj.location and input then
            self.header = self.header ..
                string.format("layout(location=%d) in ", obj.location)
        end

        if obj.type ~= "uniform" then
            self.header = self.header .. obj.type .. " "
        end
        self.header = self.header .. string.format("%s", name)
        if obj.type == "uniform" then
            self:emit_block(obj.code)
        else
            self:emit_semicolon()
        end
    end

    function glsl_src:begin_func()
    end

    function glsl_src:end_func()
    end

    function glsl_src:get_string()
        return "#version 450\n" .. self.header
        .. "\nvoid main() {\n" .. self.main
        .. "}\n"
    end

    local function is_vertex_attr(name, obj)
        local par = parser.all_stages[obj.parent]
        return par.subclass == "vertex_attribs"
    end

    local function is_uniform(name, obj)
        local par = parser.all_stages[obj.parent]
        return par.subclass == "parameter_block"
    end

    local function is_interface(name, obj)
        local par = parser.all_stages[obj.parent]
        return par.class == "interface"
    end

    -- Loop over all stages as usual
    local imported_vars = {}
    for _, stage_name in ipairs(stage_list) do
        local stage = parser.all_stages[stage_name]

        if stage.class == "interface" then
            if stage.subclass ~= "vertex_attribs" or curr_stage == "vertex" then
                for name, var in pairs(stage.outputs) do
                    -- TODO: check the stage flags of the output and curr_stage
                    glsl_src:emit_global_decl(name, var, true)
                end
            end
        elseif stage.class == "programmable" and stage.stage == curr_stage then
            for name, var in pairs(stage.inputs) do
                if var.used_export and not imported_vars[name] then
                    glsl_src:emit_global_decl(name, var, true)
                    imported_vars[name] = var
                end
            end
            for name, var in pairs(stage.outputs) do
                glsl_src:emit_global_decl(name, var)
            end

            glsl_src:emit_stmts_main(stage.code)
        end
    end

    print(glsl_src:get_string())
end

function codegen.make_pipeline(stage_list)
    codegen.annotate_parse_tree(stage_list)
    print_table(parser.all_stages)
    print()
    print("=================Vertex Shader=================")
    codegen.glsl_gen(stage_list, "vertex")
    print()
    print("=================Pixel Shader=================")
    codegen.glsl_gen(stage_list, "pixel")
end
