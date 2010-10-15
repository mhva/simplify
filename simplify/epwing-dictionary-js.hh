/*
   Copyright (C) 2010 Anton Mihalyov <anton@bytepaper.com>

   This  library is  free software;  you can  redistribute it  and/or
   modify  it under  the  terms  of the  GNU  Library General  Public
   License  (LGPL)  as published  by  the  Free Software  Foundation;
   either version  2 of the  License, or  (at your option)  any later
   version.

   This library  is distributed in the  hope that it will  be useful,
   but WITHOUT  ANY WARRANTY;  without even  the implied  warranty of
   MERCHANTABILITY or FITNESS  FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy  of the GNU Library General Public
   License along with this library; see the file COPYING.LIB. If not,
   write to the  Free Software Foundation, Inc.,  51 Franklin Street,
   Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef LIBSIMPLIFY_EPWING_DICTIONARY_JS_HH_
#define LIBSIMPLIFY_EPWING_DICTIONARY_JS_HH_

namespace simplify {

const char *g_default_js_implementation = \
u8R"#(
    _EscapeJsonString = (function() {
      var matchJsonSpecialCharsRe = /["\\]/g;
      var replaceFun = function($0) {
        return '\\' + $0;
      };

      return function(string) {
        return string.replace(matchJsonSpecialCharsRe, replaceFun);
      };
    })();

    function _BeginSubscript() {
        return '<sub>';
    }

    function _EndSubscript() {
        return '</sub>';
    }

    function _BeginSuperscript() {
        return '<sup>';
    }

    function _EndSuperscript() {
        return '</sup>';
    }

    function _BeginNoBreak() {
        return '<span style="white-space: nowrap">';
    }

    function _EndNoBreak() {
        return '</span>';
    }

    function _Indent(amount) {
        return '　　';
    }

    function _Newline() {
        return '<br/>';
    }

    function _BeginEmphasis() {
        return '<em>';
    }

    function _EndEmphasis() {
        return '</em>';
    }

    function _BeginReference() {
        return '';
    }

    function _EndReference(page, offset) {
        return '';
    }

    function _BeginKeyword() {
        return '';
    }

    function _EndKeyword() {
        return '';
    }

    function _BeginDecoration() {
        return '';
    }

    function _EndDecoration() {
        return '';
    }

    function _InsertTextGaiji() {
        return '';
    }

    function _InsertHeadingGaiji() {
        return '';
    }

    function _ProcessHeading(heading) {
        return _EscapeJsonString(heading);
    }

    function _ProcessTags(heading) {
        return '';
    }

    function _ProcessText(text) {
        return _EscapeJsonString(text);
    }
)#";

}  // namespace simplify

#endif  // LIBSIMPLIFY_EPWING_DICTIONARY_JS_HH_
