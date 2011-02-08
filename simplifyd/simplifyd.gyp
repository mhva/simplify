{
  'targets': [
    {
      'includes': [
        '../gypi/os.gypi',
      ],
      'target_name': 'simplifyd',
      'type': 'executable',
      'configurations': {
        'Debug': {
          'conditions': [
            ['OS != "win"', {
              'cflags': ['-O0', '-g'],
            }],
            ['OS == "linux"', {
              'defines': [
                # TODO: Find a better way to determine absolute path
                # to the simplifyd src directory.
                'SIMPLIFY_WWWROOT="<!(pwd)/html"',
              ],
              'cflags': ['-O3'],
            }],
          ],
        },
        'Release': {
          'defines': ['NDEBUG'],
          'conditions': [
            ['OS != "win"', {
              'cflags': ['-O3'],
            }],
            ['OS == "linux"', {
              'defines': [
                'SIMPLIFY_WWWROOT="/usr/share/simplify/static"',
              ],
              'cflags': ['-O3'],
            }],
          ],
        },
      },
      'include_dirs': [
        '<(DEPTH)',
      ],
      'sources': [
        'articleaction.cc',
        'contextaction.cc',
        'hash.cc',
        'httpquery.cc',
        'httpresponse.cc',
        'main.cc',
        'mongoose.c',
        'searchaction.cc',
        'server.cc',
      ],
      'dependencies': [
        '../simplify/simplify.gyp:libsimplify'
      ],
      'conditions': [
        ['OS != "win"', {
          'cflags': [
            '-std=c++0x', '-fstrict-aliasing', '-Wall',
            '-Wstrict-aliasing=2',
          ],
        }],
        ['OS == "linux"', {
          'defines': [
            'SIMPLIFY_CONFIG_PATH=".config/simplify"'
          ],
        }],
      ]
    },
  ]
}
