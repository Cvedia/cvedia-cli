
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
    print("Can handle: " .. feature)
    if     feature == "resume" then return true
    elseif feature == "integrity" then return true
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

    -- @TODO: Resume
    if mode == 0 then
        for key, value in pairs(set_list_table) do
            writer[value] = options["working_dir"] .. value .. ".lua"
        end
    end
    
    -- debug, requires     luarocks install inspect and local inspect = require('inspect')
    inspect = require('inspect')
    print("final data")
    print(inspect(g_options))
    print(inspect(output_fields))
    print(inspect(writer))


    return true
end

function begin_writing()
    files = {}
    inspect = require('inspect')
    print("Being writing")
    print(inspect(g_options))
    print(inspect(output_fields))
    print(inspect(writer))

    -- @TODO: Resume dont truncate
    for key, value in pairs(writer) do
        files[key] = io.open(value, "w+")
    end

    return true
end

function end_writing()
    for key, value in pairs(writer) do
        io.close(files[key])
    end

    return true
end

function write_data(entity)
    inspect = require('inspect')
    print(inspect(entity))
    return ""
end

function check_integrity(filename)
    -- global file_itr
    -- global cur_filename
    -- global g_options

    -- if cur_filename != filename:
    --     cur_filename = filename
    --     file_itr = tf.python_io.tf_record_iterator(g_options["working_dir"] + filename)

    -- for record in file_itr:

    --     m = hashlib.md5()
    --     m.update(record)

    --     return "hash=" + m.hexdigest()

    -- return "result=eof"
end

function finalize()
    return true
end