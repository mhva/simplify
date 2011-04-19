{
  'variables': {
    'simplify_library_type%': 'static_library',
    'library%': 'static_library'
  },
  'targets': [
    {
      'includes': [
        '../gypi/os.gypi',
      ],
      'target_name': 'libsimplify',
      'type': '<(simplify_library_type)',
      'configurations': {
        'Debug': {
          'conditions': [
            ['OS != "win"', {
              'cflags': ['-O0', '-g'],
            }, {
            }],
          ],
        },
        'Release': {
          'defines': ['NDEBUG'],
          'conditions': [
            ['OS != "win"', {
              'cflags': ['-O3'],
            }, {
            }],
          ],
        },
      },
      'sources': [
        'epwing/epwing-dictionary.cc',
        'config.cc',
        'dictionary.cc',
        'error.cc',
        'repository.cc',
        'utils.cc',
      ],
      'include_dirs': [
        '<(DEPTH)',
      ],
      'dependencies': [
        '../third_party/libeb/libeb.gyp:libeb',
        '../third_party/v8/tools/gyp/v8.gyp:v8',
      ],
      'conditions': [
        ['OS == "linux"', {
          'cflags': [
            '-std=c++0x', '-fstrict-aliasing', '-Wall',
            '-Wstrict-aliasing=2', '-pedantic'
          ],
          'libraries': [
            '-lrt'
          ],
        }],
      ]
    }
  ]
}
