codegen = {}

function codegen.make_pipeline(stage_list)
    for _, stage in ipairs(stage_list) do
        print(stage)
    end
end
