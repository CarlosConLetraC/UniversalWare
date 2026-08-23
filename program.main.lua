import("cjob")

cjob.new(function()
    for _ = 1, 5, 1 do
        io.write("1\n")
        io.write("3\n")
        io.write("5\n")
        io.write("7\n")
        io.write("9\n")
    end
end)

cjob.new(function()
    for _ = 1, 5, 1 do
        io.write("2\n")
        io.write("4\n")
        io.write("6\n")
        io.write("8\n")
        io.write("10\n")
    end
end)

cjob.async()
