test_inc = include_directories('../../include')

test_dep = [
    catch2,
    fmt,
    gtkmm,
    jsoncpp,
    spdlog,
]

test_src = files(
    '../main.cpp',
    'client.cpp',
    '../../src/modules/sway/ipc/client.cpp',
    '../../src/util/prepare_for_sleep.cpp',
)

sway_test = executable(
    'sway_test',
    test_src,
    dependencies: test_dep,
    include_directories: test_inc,
)

test(
    'sway',
    sway_test,
    workdir: meson.project_source_root(),
)
