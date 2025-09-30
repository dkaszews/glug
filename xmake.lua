-- Provided as part of glug under MIT license, (c) 2025 Dominik Kaszewski
set_version('3.0.1')

add_rules('mode.release', 'mode.releasedbg', 'mode.debug',  'mode.coverage')
set_allowedmodes('release', 'releasedbg', 'debug', 'coverage')
set_languages('c++17')
set_warnings('all', 'extra', 'pedantic', 'error')

add_requires('gtest >= 1.16.0')

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
    add_packages('gtest')
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

