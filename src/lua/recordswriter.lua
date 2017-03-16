
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
    for key, value in pairs(set_list_table) do
        writer[value] = options["working_dir"] .. value .. ".lua"
        if mode == 0 then
            print("truncating files")
            --truncate files
            file = os.remove(writer[value])
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
    inspect = require('inspect')
    files = {}

    print("Begin writing")
    print(inspect(g_options))
    print(inspect(output_fields))
    print(inspect(writer))

    for key, value in pairs(writer) do
        files[key] = io.open(value, "a+")
    end

    return true
end

function end_writing()
    for key, value in pairs(writer) do
        io.close(files[key])
    end

    return true
end

function write_data(entry)
    local md5 = require 'md5'
    local serpent = require("serpent")

    -- record = {}

    -- for key, entry in pairs(entry.entries) do
    --     out_field = output_fields[entry.id]

    --     if entry.value_type == "image" then
    --       --  record[out_field] = _bytes_feature(field['imagedata'].tobytes())
    --     elseif  entry.value_type == "raw" then
    --     elseif  entry.value_type == "numeric" then
    --         if entry.dtype == "int" then
    --         elseif entry.dtype == "float" then
    --         else 
    --             print("RecordsWriter does not support dtype '" + entry.dtype + "' for value type '" + entry.value_type + "'")
    --         end
    --     elseif  entry.value_type == "string" then
    --     else 
    --         print("RecordsWriter does not support '" + entry.value_type + "'")
    --     end
    -- end -- end for loop

    -- example = tf.train.Example(features=tf.train.Features(feature=record))
    -- data = example.SerializeToString()
    -- writer[entry['set']].write(data)

    -- return "file=" + entry['set'] + ".tfr;hash=" + m.hexdigest()
    serialized = serpent.line(entry)
    files[entry.set]:write(serialized, "\n");

    return "file=" .. writer[entry.set] .. ";hash=" .. md5.sumhexa(serialized) 
end

function check_integrity(filename)
    local serpent = require("serpent")
    local md5 = require 'md5'

    if cur_filename ~= filename then
        cur_filename = filename
        file = io.open(cur_filename, "r+")
    end


    line = file:read("*l")
    if line  then
        return "hash=" .. md5.sumhexa(line)
    end

    return "result=eof"
end

function finalize()
    return true
end