set_version('3.0.1')

add_rules('mode.release', 'mode.releasedbg', 'mode.debug',  'mode.coverage')
set_allowedmodes('release', 'releasedbg', 'debug', 'coverage')
set_languages('c++17')
set_warnings('all', 'extra', 'pedantic', 'error')

local engines = {
    re2 = '>= 2025.08.12',
    pcre2 = '>= 10.44',
    hyperscan = '>= 5.4.2',
}

option('engine')
    set_description('Choose regex engine')
    set_values('stl', 'pcre2', 're2', 'hyperscan')
    set_default('stl')
    set_showmenu(true)

    after_check(function (option)
        option:add('defines', 'ENGINE_' .. option:value():upper() .. '=1')
    end)
option_end()

add_requires('gtest >= 1.16.0')
if engines[get_config('engine')] then
    local engine = get_config('engine')
    add_requires(engine .. ' ' .. engines[engine])
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

target('glug_test')
    set_kind('binary')
    add_tests('default')
    add_files('src/**.cpp', 'test/**.cpp')
    add_includedirs('include', 'test')
    add_defines('UNIT_TEST=1')
    add_packages('gtest', get_config('engine'))
    add_options('engine')
target_end()

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
task_end()

