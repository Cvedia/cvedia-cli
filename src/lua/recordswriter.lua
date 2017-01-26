
function load_module ()
    local mod = {}
    mod.module_name = 'LuaRecordsWriter'
    mod.params = { 
        {required= false, option= "test-param", example= "  --test-param <num>", description= "Fake parameter that does nothing. Default 0"},
        {required= false, option= "test-param2", example= "  --test-param2 <num>", description= "Fake parameter that does nothing again. Default 0"},
    }
    return mod
end


function can_handle( feature )
    if     feature == "a" then return true
    elseif feature == "b" then return true
    end

    return false

end

function initialize(options, field_list_table, set_list_table, mode)
    writer = {}
    output_fields = {}
    g_options = options


    for key, value in pairs(field_list_table) do
        output_fields[value["id"]] = value["name"]
    end

    if mode == 0 then
        for key, value in pairs(set_list_table) do
            writer[value] = options["working_dir"] .. value .. ".lua"
        end
    end
    
    -- debug, requires     luarocks install inspect and local inspect = require('inspect')
    -- print("final data")
    -- print(inspect(g_options))
    -- print(inspect(output_fields))
    -- print(inspect(writer))

    -- for field in dataset_meta['fields']:
    --     output_fields[field['id']] = field['name']

    -- # Only create new TFR files if we are in MODE_NEW
    -- if mode == 0:
    --     for dataset in dataset_meta['sets']:
    --         writer[dataset] = tf.python_io.TFRecordWriter(options["working_dir"] + dataset + ".tfr")

    return true
end

function begin_writing()
    return true
end

function end_writing()
    return true
end

function write_data(entity)
    return true
end

function check_integrity(filename)
    return true
end

function finalize()
    return true
end