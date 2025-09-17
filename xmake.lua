set_version('3.0.1')

add_rules('mode.release', 'mode.releasedbg', 'mode.debug',  'mode.coverage')
set_allowedmodes('release', 'releasedbg', 'debug', 'coverage')
set_languages('c++17')
set_warnings('all', 'extra', 'pedantic', 'error')

option('engine')
    set_description('Choose regex engine')
    set_values('stl', 'pcre2', 're2', 'hyperscan', 'vectorscan')
    set_default('stl')
    set_showmenu(true)
option_end()

-- TODO: Move to UT targets so it is not required to build main
add_requires('gtest >= 1.16.0')

local engines = {
    re2 = { version = '>= 2025.08.12', define = 'ENGINE_RE2=1' },
    pcre2 = { version = '>= 10.44', define = 'ENGINE_PCRE2=1' },
    -- TODO: May (?) require openmpi to install, but only in debug (?!)
    hyperscan = { version = '>= 5.4.2', define = 'ENGINE_HYPERSCAN=1' },
}

local add_engine = function() end
for package, data in pairs(engines) do
    if is_config('engine', package) then
        add_requires(package .. ' ' .. data.version)
        add_engine = function()
            add_packages(package)
            add_defines(data.define)
        end
    end
end

-- https://github.com/xmake-io/xmake/issues/5769
if is_mode('coverage') then
    set_policy('build.ccache', false)
end

-- Not enabled in coverage as they generate extra branches
if is_mode('debug') and is_os('linux') then
    for _, sanitizer in ipairs({ 'address', 'undefined', 'leak' }) do
        set_policy('build.sanitizer.' .. sanitizer, true)
    end
end

-- Don't be annoying when actively writing code
if is_mode('debug', 'coverage') and not getenv('CI') then
    add_cxxflags(
        '-Wfatal-errors',
        '-Wno-unused-function',
        '-Wno-unused-parameter',
        '-Wno-unused-variable',
        '-Wno-unused-but-set-variable',
        '-Wno-unused-local-typedefs',
        { tools = { 'gcc', 'clang' } }
    )
end

target('glug')
    set_kind('binary')
    add_files('src/**.cpp')
    add_includedirs('include')
    add_engine()

target('glug_test')
    set_kind('binary')
    add_files('src/**.cpp|main.cpp', 'test/src/**.cpp')
    add_includedirs('include', 'test/include')
    add_defines('UNIT_TEST=1', 'MOCK_FS=1')
    add_packages('gtest')
    add_engine()
    add_tests('default')

-- TODO: merge with above, add config option
target('glug_test_fs')
    set_kind('binary')
    add_files('src/**.cpp|main.cpp', 'test/src/*.cpp', 'test/src/filesystem/**.cpp')
    add_includedirs('include', 'test/include')
    add_defines('UNIT_TEST=1')
    add_packages('gtest')
    add_engine()
    add_tests('default')

task('coverage')
    on_run(function (target)
        os.rm('**/*.gcda')
        os.exec('xmake config --mode=coverage')
        os.exec('xmake build -v')
        os.exec('xmake test -v')
        os.execv('gcovr', {}, { try = true })
        os.exec('gcovr --html-details --html-single-page --output coverage_report.html')
    end)
    set_menu {}

