import("cjob", "system")

cjob.new(system.print, "ok", {3, 6, 9}, math.cos, true)

cjob.async()

--[[local io_write = io.write
cjob.new(function()
    for _ = 1, 5, 1 do
        io_write("1\n")
        io_write("3\n")
        io_write("5\n")
        io_write("7\n")
        io_write("9\n")
    end
end)

cjob.new(function()
    for _ = 1, 5, 1 do
        io_write("2\n")
        io_write("4\n")
        io_write("6\n")
        io_write("8\n")
        io_write("10\n")
    end
end)

cjob.async()]]

