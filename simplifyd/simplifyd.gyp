{
  'includes': [
    '../gypi/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'simplifyd',
      'type': 'executable',
      'sources': [
        'articleaction.cc',
        'contextaction.cc',
        'hash.cc',
        'httpquery.cc',
        'httpresponse.cc',
        'main.cc',
        'mongoose.c',
        'options.cc',
        'searchaction.cc',
        'server.cc',
      ],
      'conditions': [
        ['OS == "linux"', {
          'defines': ['SIMPLIFY_CONFIG_PATH=".config/simplify"'],
          'cflags': ['-std=c++0x', '-fstrict-aliasing', '-Wstrict-aliasing=2'],
        }],
      ],
      'configurations': {
        'Debug': {
          'conditions': [
            ['OS == "linux"', {
              # TODO: Find a better way to determine absolute path
              # to the simplifyd src directory.
              'defines': ['SIMPLIFY_WWWROOT="<!(pwd)/html"'],
            }],
          ],
        },
        'Release': {
          'conditions': [
            ['OS == "linux"', {
              'defines': ['SIMPLIFY_WWWROOT="/usr/share/simplify/static"'],
            }],
          ],
        },
      },
      'dependencies': [
        '../simplify/simplify.gyp:libsimplify'
      ],
      'include_dirs': [
        '<(DEPTH)',
      ],
    },
  ]
}
# vim: set et ts=2 sw=2:
