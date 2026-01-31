-- Provided as part of glug under MIT license, (c) 2025-2026 Dominik Kaszewski
set_version('3.0.1')

add_rules('mode.release', 'mode.releasedbg', 'mode.debug',  'mode.coverage')
set_allowedmodes('release', 'releasedbg', 'debug', 'coverage')
set_languages('c++20')
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

-- Naming it `--version` interferes with package versions
option('tag')
    set_description('Binary version tag')
    set_default('')  -- nil causes `after_check` to be skipped
    set_showmenu(true)

    after_check(function (option)
        local version = option:value()
        if version == '' then
            local head, _ = os.iorunv('git', { 'rev-parse', 'HEAD' })
            version = '0.0.0+' .. head:sub(0, 12)
        end
        option:add('defines', 'GLUG_VERSION="' .. version .. '"')
    end)
option_end()

add_requires('gtest >= 1.16.0')
if regex_engines[get_config('regex')] then
    local engine = get_config('regex')
    local config = { configs = { shared = is_kind('shared') } }
    add_requires(engine .. ' ' .. regex_engines[engine], config)
end

-- GCC 15.1.0 cross-toolchain complains about gtest using `<ciso646>`
if is_cross('riscv64-linux-gnu-') then
    add_cxxflags('-Wno-cpp')
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

function generate_licenses(target)
    function gen(input)
        local base = input:sub(1, -path.extension(input):len() - 1):lower()
        local output = 'include/glug/generated/' .. base .. '.hpp'
        local outfile = io.open(output, 'w')

        local namespace = base:gsub(path.sep(), '::')
        outfile:write('#pragma once\n')
        outfile:write('// NOLINTBEGIN\n')
        outfile:write('\n')
        outfile:write('namespace glug::generated::' .. namespace .. ' {\n')
        outfile:write('\n')
        outfile:write('static constexpr const char* data =\n')
        outfile:write('R"---(')

        local infile = io.open(input, 'r')
        for line in infile:lines() do
            outfile:write(line .. '\n')
        end
        infile:close()

        outfile:write(')---";\n')
        outfile:write('\n')
        outfile:write('}\n')
        outfile:write('// NOLINTEND\n')
        outfile:write('\n')
        outfile:close()
    end

    gen('LICENSE.md')
    for _, license in ipairs(os.files('licenses/*.md')) do
        gen(license)
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
    add_options('regex', 'tag')
    before_build(generate_licenses)
    after_build(copy_latest)
target_end()

target('unit_test')
    set_kind('binary')
    add_tests('default')
    add_files('src/**.cpp|main.cpp', 'test/unit/**.cpp')
    add_includedirs('include', 'test')
    add_defines('UNIT_TEST=1')
    add_packages('gtest', get_config('regex'))
    add_options('regex', 'tag')
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

task('suffix')
    on_run(function (target)
        import('core.project.config')
        import('core.base.option')
        config.load()

        local version = config.get('tag')
        if version == '' then
            local head, _ = os.iorunv('git', { 'rev-parse', 'HEAD' })
            version = '0.0.0+' .. head:sub(0, 12)
        end

        local name = version
        function append_if_not(value, skip)
            if value ~= skip then
                name = name .. '-' .. value
            end
        end

        append_if_not(config.get('target_os') or config.get('plat'), nil)
        append_if_not(config.get('arch'), nil)
        append_if_not(config.get('mode'), 'release')
        append_if_not(config.get('kind'), 'shared')
        append_if_not(config.get('regex'), 'stl')
        print(name)
    end)
    set_menu {}
task_end()

