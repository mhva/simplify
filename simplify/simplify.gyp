{
  'variables': {
    'simplify_library_type%': 'static_library',
    'library%': 'static_library'
  },
  'includes': [
    '../gypi/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'libsimplify',
      'type': '<(simplify_library_type)',
      'sources': [
        'epwing/epwing-dictionary.cc',
        'config.cc',
        'dictionary.cc',
        'error.cc',
        'repository.cc',
        'utils.cc',
      ],
      'conditions': [
        ['OS == "linux"', {
          'cflags': ['-std=c++0x', '-fstrict-aliasing', '-Wstrict-aliasing=2'],
          'libraries': ['-lrt'],
        }],
      ],
      'dependencies': [
        '../third_party/libeb/libeb.gyp:libeb',
        '../third_party/v8/tools/gyp/v8.gyp:v8',
      ],
      'include_dirs': [
        '<(DEPTH)',
      ],
    }
  ]
}
# vim: set et ts=2 sw=2:
