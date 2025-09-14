set_version('3.0.1')

add_rules('mode.debug', 'mode.release', 'mode.coverage')
add_requires('gtest >= 1.16.0')
set_languages('c++17')
set_warnings('all', 'extra', 'pedantic', 'error')

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
if not is_mode('release') and not getenv('CI') then
    add_cxxflags(
        '-Wno-unused-function',
        '-Wno-unused-parameter',
        '-Wno-unused-variable',
        '-Wno-unused-but-set-variable',
        '-Wno-unused-local-typedefs',
        { tools = { 'gcc', 'clang' } }
    )
end

-- target('glug')
--     set_kind('binary')
--     add_files('src/**.cpp')
--     add_includedirs('include')

target('glug_test')
    set_kind('binary')
    add_files('src/**.cpp|main.cpp', 'test/src/**.cpp')
    add_includedirs('include', 'test/include')
    add_defines('UNIT_TEST=1', 'MOCK_FS=1')
    add_packages('gtest')
    add_tests('default')

task('coverage')
    on_run(function (target)
        os.rm('**/*.gcda')
        os.exec('xmake config --mode=coverage')
        os.exec('xmake build -v glug_test')
        os.exec('xmake test -v')
        os.execv('gcovr', {}, { try = true })
        os.exec('gcovr --html-details --html-single-page --output coverage_report.html')
    end)
    set_menu {}

