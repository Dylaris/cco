local luabc = require("luabc")
local cmd   = luabc.cmd
local tool  = luabc.tool
local debug = luabc.debug

local CC        = "gcc"
local ASM       = "fasm"
local CFLAGS    = { "-Wall", "-Wextra", "-I src/" }
local CSTD      = "-std=c11"
local TARGET    = "main"
local C_SRC     = tool.match_file_extension(".c", "src")
local ASM_SRC   = tool.match_file_extension(".asm", "src")
local OBJ       = tool.replace_files_extension({C_SRC, ASM_SRC}, ".o")
local CLEAR     = { TARGET, table.unpack(OBJ) }

local function build()
    -- compile
    for i, _ in ipairs(C_SRC) do
        local compile = cmd:new()
        compile:append(CC, CFLAGS, CSTD, "-c", "-o", OBJ[i], C_SRC[i])
    end
    for i, _ in ipairs(ASM_SRC) do
        local compile = cmd:new()
        compile:append(ASM, ASM_SRC[i], OBJ[#C_SRC+i])
    end

    -- link
    local link = cmd:new()
    link:append(CC, CFLAGS, CSTD, "-o", TARGET, OBJ)

    local clean = cmd:new("clean")
    clean:call(tool.clean, CLEAR)

    luabc.build()
end
build()

