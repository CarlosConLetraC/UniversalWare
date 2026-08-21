import("cjob")

cjob.new(function()
    for _ = 1, 5 do
        io.write("pe")
    end
end)

cjob.new(function()
    for _ = 1, 5 do
        io.write("ne\n")
    end
end)

cjob.new(function()
    for _ = 1, 5 do
        io.write("vag")
    end
end)

cjob.new(function()
    for _ = 1, 5 do
        io.write("ina\n")
    end
end)

cjob.async()
