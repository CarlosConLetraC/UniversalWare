import("cjob")

cjob.new(function()
    for i = 1, 100, 1 do
        print("función sincronizada por step-by-step mediante procesamiento de opcodes LuaJIT.")
    end
end)

cjob.async()