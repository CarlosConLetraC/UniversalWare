import("cmariadb", "system")

local SqlValue = cmariadb.SqlValue
local geometries = {
    POINT              = SqlValue.geometry("POINT(19.4326 -99.1332)"),
    LINESTRING         = SqlValue.geometry("LINESTRING(0 0, 1 1, 2 2, 3 3)"),
    POLYGON            = SqlValue.geometry("POLYGON((0 0, 4 0, 4 4, 0 4, 0 0), (1 1, 2 1, 2 2, 1 2, 1 1))"),
    MULTIPOINT         = SqlValue.geometry("MULTIPOINT(0 0, 1 2, 3 4)"),
    MULTILINESTRING    = SqlValue.geometry("MULTILINESTRING((0 0, 1 1, 2 2), (3 3, 4 4, 5 5))"),
    GEOMETRYCOLLECTION = SqlValue.geometry(
        "GEOMETRYCOLLECTION(" ..
            "POINT(19.4326 -99.1332), " ..
            "LINESTRING(0 0, 5 5, 10 10), " ..
            "POLYGON((0 0, 10 0, 10 10, 0 10, 0 0)), " ..
            "MULTIPOINT(1 1, 2 2, 3 3)," ..
            "GEOMETRYCOLLECTION(" .. -- Colección anidada
                "LINESTRING(0 0, 5 5), " ..
                "POINT(1 1)" ..
            ")"..
        ")"
    )
}

Table.settings.PAGE_LIMIT = 1
for k, v in pairs(geometries) do
    system.print("--- " .. k .. " ---")
    local coords = assert(v:castcoordinates(), "failed to cast geometry '"..k.."'.")
    system.print(coords)
end

local test = SqlValue.string("Hola")
system.print(debug.getmetatable(test))
system.print(debug.getmetatable(geometries.GEOMETRYCOLLECTION))