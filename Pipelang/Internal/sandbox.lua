sandbox = {}

local PL_CLASS_MT = {}
PL_CLASS_MT.__call = function (self, arg)
    if not rawget(self, "_argc") then
        rawset(self, "_argc", 0)
    end

    local c = rawget(self, "_argc")
    rawset(self, "a" .. c, arg)
    c = c + 1
    rawset(self, "_argc", c)

    return self
end

PL_CLASS_MT.__index = function (self, key)
    return function (self, arg)
        rawset(self, key, arg)
        return self
    end
end

local PL_ENV_MT = {}
PL_ENV_MT.__index = function (t, key)
    if _G[key] then
        return _G[key]
    end
    if key == "VertexAttribs" or key == "ParameterBlock" or key == "Rasterizer" or key == "DepthStencil" or key == "Blend" or key == "GeometryShader" then
        return function (name)
            local tbl = setmetatable({ type = key, name = name }, PL_CLASS_MT)
            sandbox[name] = tbl
            return tbl
        end
    else
        return setmetatable({ type = key }, PL_CLASS_MT)
    end
end

setmetatable(sandbox, PL_ENV_MT)

loadfile(internal_path .. "/main.lua", "bt", sandbox)()

function print_table(t)
    io.write("{")
    for k, v in pairs(t) do
        io.write("\"" .. k .. "\": ")
        if type(v) == "table" then
            print_table(v)
        else
            io.write("\"")
            io.write(tostring(v))
            io.write("\"")
        end
        io.write(",")
    end
    io.write("}")
end
-- print_table(sandbox)
