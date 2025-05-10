local LUABC_DIR = "../"
package.path = package.path .. ";" .. LUABC_DIR .. "?.lua"

local luabc = require("luabc")
local cmd   = luabc.cmd
local tool  = luabc.tool
local debug = luabc.debug

local CC        = "gcc"
local ASM       = "fasm"
local CFLAGS    = { "-Wall", "-Wextra", "-I ../src/", "-I ../" }
local CSTD      = "-std=c11"
local EXAMPLE   = tool.match_file_extension(".c")
local TARGET    = tool.replace_files_extension(EXAMPLE, (luabc.os == "WIN") and ".exe" or "")
local C_SRC     = tool.match_file_extension(".c", "../src/")
local ASM_SRC   = tool.match_file_extension(".asm", "../src/")
local OBJ       = tool.replace_files_extension({C_SRC, ASM_SRC}, ".o")
local CLEAR     = { TARGET, table.unpack(OBJ) }

local function build()
    -- compile
    for i, _ in ipairs(ASM_SRC) do
        local compile = cmd:new()
        compile:append(ASM, ASM_SRC[i], OBJ[#C_SRC+i])
    end
    for i, _ in ipairs(C_SRC) do
        local compile = cmd:new()
        compile:append(CC, CFLAGS, CSTD, "-c", "-o", OBJ[i], C_SRC[i])
    end

    -- link
    for i, _ in ipairs(TARGET) do
        local link = cmd:new()
        link:append(CC, CFLAGS, CSTD, "-o", TARGET[i], EXAMPLE[i], OBJ)
    end

    local clean = cmd:new("clean")
    clean:call(tool.clean, CLEAR)

    -- debuging
    local debugging = cmd:new("debug", nil, true)
    debugging:call(function () table.insert(CFLAGS, "-g") end)
    debugging:link()

    luabc.build()
end
build()

