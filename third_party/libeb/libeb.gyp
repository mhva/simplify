{
  'variables': {
    'libeb_library_type%': 'static_library'
  },
  'targets': [
    {
      'target_name': 'libeb',
      'type': '<(libeb_library_type)',
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
          'conditions': [
            ['OS != "win"', {
              'cflags': ['-O2'],
            }, {
            }]
          ],
        },
      },
      'sources': [
        'eb/appendix.c',
        'eb/appsub.c',
        'eb/bcd.c',
        'eb/binary.c',
        'eb/bitmap.c',
        'eb/book.c',
        'eb/booklist.c',
        'eb/copyright.c',
        'eb/cross.c',
        'eb/eb.c',
        'eb/endword.c',
        'eb/error.c',
        'eb/exactword.c',
        'eb/filename.c',
        'eb/font.c',
        'eb/hook.c',
        'eb/jacode.c',
        'eb/keyword.c',
        'eb/lock.c',
        'eb/log.c',
        'eb/match.c',
        'eb/menu.c',
        'eb/multi.c',
        'eb/narwalt.c',
        'eb/narwfont.c',
        'eb/readtext.c',
        'eb/search.c',
        'eb/setword.c',
        'eb/stopcode.c',
        'eb/strcasecmp.c',
        'eb/subbook.c',
        'eb/text.c',
        'eb/widealt.c',
        'eb/widefont.c',
        'eb/word.c',
        'eb/zio.c'
      ],
      'defines': [
        'EB_BUILD_LIBRARY'
      ],
      'direct_dependent_settings': {
        'include_dirs': ['.'],
      },
      'link_settings': {
        'conditions': [
          ['OS != "win"', {
            'libraries': ['-lz'],
          }],
        ],
      },
      'conditions': [
        ['OS != "win"', {
          'libraries': ['-lz'],
        }],
      ],
    }
  ]
}
