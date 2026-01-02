-- Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
set_version('3.0.1')

add_rules('mode.release', 'mode.releasedbg', 'mode.debug',  'mode.coverage')
set_allowedmodes('release', 'releasedbg', 'debug', 'coverage')
set_languages('c++17')
set_warnings('all', 'extra', 'pedantic', 'error')

local regex_engines = {
    re2 = '>= 2025.08.12',
    pcre2 = '>= 10.44',
    hyperscan = '>= 5.4.2',
}

option('regex')
    set_description('Choose regex engine')
    set_values('stl', 'pcre2', 're2', 'hyperscan')
    set_default('stl')
    set_showmenu(true)

    after_check(function (option)
        option:add('defines', 'GLUG_REGEX_' .. option:value():upper() .. '=1')
    end)
option_end()

add_requires('gtest >= 1.16.0')
if regex_engines[get_config('regex')] then
    local engine = get_config('regex')
    add_requires(engine .. ' ' .. regex_engines[engine])
end

-- Windows assumes all strings are ANSI code pages, garbling output
if is_plat('windows', 'mingw') then
    add_cxxflags('-utf-8')
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

function copy_latest(target)
    destination = 'build/latest/'
    os.mkdir(destination)
    os.cp(target:targetfile(), destination)
end

target('glug')
    set_kind('binary')
    add_files('src/**.cpp')
    add_includedirs('include')
    add_packages(get_config('regex'))
    add_options('regex')
    after_build(copy_latest)
target_end()

target('unit_test')
    set_kind('binary')
    add_tests('default')
    add_files('src/**.cpp|main.cpp', 'test/unit/**.cpp')
    add_includedirs('include', 'test')
    add_defines('UNIT_TEST=1')
    add_packages('gtest', get_config('regex'))
    add_options('regex')
    after_build(copy_latest)
target_end()

target('parity_test')
    set_kind('phony')
    add_tests('default')
    add_deps('glug')
    on_test(function (target, opt)
        if import('core.base.option').get('verbose') then
            os.execv('pytest', { 'test/parity', '-v' })
        else
            os.execv('pytest', { 'test/parity' }, { stdout = os.tmpfile() })
        end
        return true
    end)
target_end()

task('coverage')
    on_run(function (target)
        os.rm('build/**/*.gcda')
        os.execv('xmake', { 'build', '-v', 'unit_test/default' })
        os.execv('xmake', { 'test', '-v', 'unit_test/default' })
        os.execv('gcovr', {
            '--txt',
            '--html-single-page',
            '--html-details',
            'coverage_report.html',
            '--json-pretty',
            '--json',
            'coverage_report.json'
        })
    end)
    set_menu {}
task_end()

