
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

    io.write("initialize from lua!!")
    print("initialize from luaasdasda")

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