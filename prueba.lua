import("cjob")

cjob.new(function()
    for i = 1, 10, 1 do
        if i % 2 == 0 then
            print("par: "..i)
        end
    end
end)
cjob.new(function()
    for i = 1, 10, 1 do
        if i % 2 == 1 then
            print("impar: "..i)
        end
    end
end)


cjob.async()