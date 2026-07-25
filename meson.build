project('ListTalkEmbedded', 'c',
        version : '0.1.0',
        default_options : ['c_std=c11'])

pkg = import('pkgconfig')

add_project_arguments('-D_POSIX_C_SOURCE=200809L', language : 'c')

listtalk_embedded_sources = [
        'src/amalgamation.c',
]

listtalk_embedded_lib = library(
        'ListTalkEmbedded',
        listtalk_embedded_sources,
        include_directories : include_directories('.'),
        install : true
)

listtalk_embedded_dep = declare_dependency(
        include_directories : include_directories('.'),
        link_with : listtalk_embedded_lib
)

install_subdir('ListTalkEmbedded',
               install_dir : get_option('includedir'))

pkg.generate(
        listtalk_embedded_lib,
        filebase : 'ListTalkEmbedded',
        name : 'ListTalkEmbedded',
        description : 'ListTalk embedded runtime support library',
        subdirs : '.'
)

smoke_test_exe = executable(
        'smoke_test',
        'tests/smoke_test.c',
        dependencies : [listtalk_embedded_dep]
)

test('smoke_test', smoke_test_exe)
