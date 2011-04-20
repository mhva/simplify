{
  'conditions': [
    ['OS == "linux"', {
      'target_defaults': {
        'defines': [
          'SIMPLIFY_POSIX',
          'SIMPLIFY_LINUX',
        ],
        'cflags': ['-Wall', '-pedantic'],
        'configurations': {
          'Debug': {
            'cflags': ['-g'],
          },
          'Release': {
            'defines': ['NDEBUG'],
            'cflags': ['-O3'],
          }
        },
      },
    }],
  ]
}
# vim: set et ts=2 sw=2:
