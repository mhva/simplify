// ====================================================================
// CharMapper.
// ====================================================================

/**
 * Maps a range of characters to a different range of characters.
 *
 * The CharMapper accepts a variadic argument list where each argument is an
 * array with 3 elements: [<first_src_char_code>,
 *                         <first_dst_char_code>,
 *                         <length>]
 * <first_src_char_code> - Character code of the first character in the source
 *  range.
 * <first_dst_char_code> - Character code of the first character in the
 *  destination range.
 * <length> - Range length.
 */
function CharMapper() {
  this.ranges = [];

  for (var i = 0; i < arguments.length; i++) {
    // TODO: Add sanity checks for range.
    this.ranges.push(arguments[i]);
  }
}

CharMapper.prototype = {
  /**
   * Maps a source character code to an associated destination character code.
   *
   * \return If succeeds, returns a destination character code. If fails,
   *  returns undefined.
   */
  MapToCode: function(charCode) {
    for (var i = 0; i < this.ranges.length; i++) {
      var range = this.ranges[i];

      if (charCode >= range[0] && charCode < range[0] + range[2]) {
        return range[1] + (charCode - range[0]);
      }
    }

    return undefined;
  },

  /**
   * Maps a source character code to an associated destination character.
   *
   * \return If succeeds, returns a string of length 1 with destination
   *  character in it. If fails, returns undefined.
   */
  MapToChar: function(charCode) {
    var result = this.MapToCode(charCode);
    return (result !== undefined) ? String.fromCharCode(result) : undefined;
  },

  /**
   * Returns a string that can be used in a regular expression to match a
   * full range of destination characters.
   *
   * Example:
   *    var cm = new CharMapper([0x00, 0x30, 10]); // Destination: '0'-'9'
   *    print(cm.GetRegexpRange);
   *
   * Prints:
   *    0-9
   */
  GetRegexpRange: function() {
    var text = '';
    for (var i = 0; i < this.ranges.length; i++) {
      var range = this.ranges[i];

      if (range[2] > 1) {
        text = text + String.fromCharCode(range[1]) + '-' +
               String.fromCharCode(range[1] + range[2] - 1);
      } else {
        text = text + range[1];
      }
    }

    return text;
  },

  _GetCharCodeDelta: function(charCode, srcIndex) {
    var delta = 0;
    for (var i = 0; i < this.ranges.length; i++) {
      var range = this.ranges[i];
      var srcCode = range[srcIndex];

      if (charCode >= srcCode && charCode < srcCode + range[2]) {
        return delta + (charCode - srcCode);
      } else {
        delta += range[2];
      }
    }

    return undefined;
  },

  /**
   * Returns offset in the source character range for a character with
   * code @charCode.
   */
  GetSrcCharCodeDelta: function(charCode) {
    return this._GetCharCodeDelta(charCode, 0);
  },

  /**
   * Returns offset in the destination character range for a character with
   * code @charCode.
   */
  GetDstCharCodeDelta: function(charCode) {
    return this._GetCharCodeDelta(charCode, 1);
  },

  GetDstCharDelta: function(c) {
    return this._GetCharCodeDelta(c.charCodeAt(0), 1);
  },

  /**
   * Returns a string containing one character with index @index from
   * destination range.
   */
  GetDstCharWithIndex: function(index) {
    var delta = 0;

    for (var i = 0; i < this.ranges.length; i++) {
      var range = this.ranges[i];

      if (index < delta + range[2]) {
        return String.fromCharCode(range[1] + (index - delta));
      } else {
        delta += range[2];
      }
    }

    return undefined;
  }
};

// ====================================================================
// Global Gaiji mappings.
// ====================================================================

var gaijiMap = {
  0xb467: '➡',
  0xb560: '',
  0xb55f: '',
  0xb468: '⇔'
};

// ====================================================================
// TextProcessor.
// ====================================================================

function TextProcessor() {}
TextProcessor.prototype = {
  _DecorateTitle: (function() {
    var matchTitleRe = /^\s*(.+?)<br\/>\s*/;
    var matchGarbageRe = /［.*?］/g;
    var replaceFun = function($0, $1) {
      return '<div class="article-title">' +
        $1.replace(matchGarbageRe, '').replace('：', '') + '</div>';
    };

    return function(text) {
      return text.replace(matchTitleRe, replaceFun);
    };
  })(),

  _DecorateListNumbers: (function() {
    var matchLv0Re = /［([一二三四五六七八九])］/g;
    var matchLv1Re = /（([一二三四五六七八九])）/g;

    var decorateLv0Fun = function($0, $1) {
      return '<span class="a-bln1">' + $0 + '</span>';
    };

    var decorateLv1Fun = function($0, $1) {
      return '<span class="a-bln2">' + $0 + '</span>';
    };

    return function(text) {
      text = text.replace(matchLv0Re, decorateLv0Fun);
      text = text.replace(matchLv1Re, decorateLv1Fun);
      return text;
    };
  })(),

  _DecorateExamples: (function() {
    var matchExampleRe = /「(.*?)」/g;
    var replaceFun = function($0, $1) {
      var examples = $1.split('／');

      if (examples.length > 1) {
        var html = ['<div style="margin-left: 1.3em">'];
        for (var i = 0; i < examples.length; i++) {
          html.push('<div class="a-li"><span class="a-ln">');
          html.push((i + 1).toString());
          html.push('</span><span class="a-lt">');
          html.push(examples[i]);
          html.push('</span></div>');
        }
        html.push('</div>');
        return html.join('');
      } else {
        return $0;
      }
    };

    return function(text) {
      return text.replace(matchExampleRe, replaceFun);
    };
  })(),

  ProcessText: function(text) {
    text = this._DecorateTitle(text);
    text = this._DecorateExamples(text);
    text = this._DecorateListNumbers(text);

    return _EscapeJsonString(text);
  }
};

// ====================================================================
// HeadingProcessor.
// ====================================================================

function HeadingProcessor() {}
HeadingProcessor.prototype = {
  /**
   * Joins multiple, so called 'Kanji Parts' (text enclosed into 【】) into
   * one.
   *
   * As a side effect, pronunciations will be also removed. It's not like
   * they mattered anyway.
   */
  _JoinKanjiParts: function(text) {
    var left1;
    var right1;

    if ((left1 != text.indexOf('【')) != -1 &&
        (right1 = text.indexOf('】', left1)) != -1) {
      var chunks = [text.slice(0, right1)];
      var left2;
      var right2;

      if ((left2 = text.indexOf('【', right1)) != -1 &&
          (right2 = text.indexOf('】', left2)) != -1) {
        chunks.push('・');
        chunks.push(text.slice(left2 + 1, right2));
      }

      chunks.push('】');
      return chunks.join('');
    } else {
      return text;
    }
  },

  /**
   * Translates full-width Latin characters into Basic form.
   */
  _TranslateFullWidthLatin: (function() {
    var matchFwLatinRe = /[０-ｚ]/g;
    var latinMapper = new CharMapper([0xff10, 0x0030, 74]);
    var replaceFun = function($0) {
      return latinMapper.MapToChar($0.charCodeAt(0));
    };

    return function(text) {
      return text.replace(matchFwLatinRe, replaceFun);
    };
  })(),

  /**
   * Permutes a word.
   *
   * Example: Word like '掛（け）' will result in 2 permutations:
   *    掛, 掛け
   */
  _PermuteWord: (function() {
    var matchBracketsRe = /（(.*?)）/g;
    var Permute = function(variants, index, appendTo, out) {
      if (index < variants.length) {
        for (var i = 0; i < variants[index].length; i++) {
          Permute(variants, index + 1, appendTo + variants[index][i], out);
        }
      } else {
        out.push(appendTo);
      }
    };

    return function(text, out) {
      var matches;
      var sliceFrom = 0;
      var variants = [];

      // Put each possible chunk mutation into list and push this list into
      // @variants so, later, we can make all possible permutations.
      while ((matches = matchBracketsRe.exec(text)) !== null) {
        var chunk = text.slice(sliceFrom, matches.index);

        variants.push([chunk, chunk + matches[1]])
        sliceFrom = matchBracketsRe.lastIndex;
      }

      if (variants.length == 0) {
        out.push(text);
      } else {
        if (sliceFrom < text.length) {
          variants.push([text.slice(sliceFrom)]);
        }

        Permute(variants, 0, '', out);
      }
    }
  })(),

  _StripChunksEnclosedInParentheses: (function() {
    var matchParenthesesRe = /（.*?）/g;

    return function(text) {
      return text.replace(matchParenthesesRe, '');
    }
  })(),

  ProcessHeading: (function() {

    return function(heading) {
      heading = this._JoinKanjiParts(heading);

      // HACK: Save the result so the ProcessTags can later pick it up and
      // avoid burning unneccessary CPU cycles.
      this.processedHeading = heading;

      heading = this._TranslateFullWidthLatin(heading);
      return _EscapeJsonString(heading);
    };
  })(),

  ProcessTags: (function() {
    var stripLangRe = /【（.*）　/;

    return function(heading) {
      var templSource;

      if (this.processedHeading !== undefined) {
        templSource = this.processedHeading;
        this.processedHeading = undefined;
      } else {
        // TODO: Might be a good idea to make ProcessHeading() to look for
        // cached result.
        print('Entering a slow processing path in ProcessTags function. ' +
              'Please consider calling ' +
              'Dictionary::SearchResults::FetchHeading() before calling ' +
              'Dictionary::SearchResults::FetchTags().');
        templSource = this.ProcessHeading();
      }

      if (templSource !== null) {
        var left;

        // If we have some templates- permute them and produce tags.
        if (templSource[templSource.length - 1] == '】' &&
            (left = templSource.indexOf('【')) != -1) {
          // Strip the part describing the origin language of the word in
          // the heading.
          templSource = templSource.replace(stripLangRe, '【');

          var tags = [];
          var templates =
            templSource.slice(left + 1, templSource.length - 1).split('・');

          // Permute each tag template. A tag template might contain a part that
          // can be optionally removed or inserted (a text inclosed into（）).
          // We should permute each template and produce real tags.
          for (var i = 0; i < templates.length; i++) {
            this._PermuteWord(templates[i], tags);
          }

          // XXX: Push the reading into tags as well. Might be useful,
          // but I'm not sure.
          tags.push(templSource.slice(0, left));

          return _EscapeJsonString(tags.join(','));
        } else {
          // The heading contains only a reading, so we return this reading
          // as tag.

          // Some readings might contain Kanji with their readings enclosed in
          // full-width parentheses (WTF, this is a reading for God's sake).
          // We should strip these readings to produce valid tags.
          return this._StripChunksEnclosedInParentheses(heading);
        }
      } else {
        return '';
      }
    }
  })()
};

// ====================================================================
// Hooks.
// ====================================================================

//function Indent() {
//  return '';
//}

function InsertHeadingGaiji(gaijiCode) {
  var result = gaijiMap[gaijiCode];
  return result !== undefined ? result : '?';
}

function InsertTextGaiji(gaijiCode) {
  var result = gaijiMap[gaijiCode];
  return result !== undefined ? result
                              : '&lt;0x' + gaijiCode.toString(16) + '&gt;';
}

function BeginKeyword() {
  // Keywords in DAIJIRIN appear to depict pronunciation of the word in the
  // article's title.
  return '';
  return '&lt;kw&gt;';
}

function EndKeyword() {
  return '';
  return '&lt;/kw&gt;';
}

function BeginReference() {
  // Keywords in DAIJIRIN appear to depict pronunciation of the word in the
  // article's title.
  return '';
  return '&lt;REF&gt;';
}

function EndReference() {
  return '';
}

(function() {
  var headingProcessor = new HeadingProcessor();

  ProcessHeading = function(text) {
    return headingProcessor.ProcessHeading(text);
  };

  ProcessTags = function(text) {
    return headingProcessor.ProcessTags(text);
  };
})();

ProcessText = (function() {
  var textProcessor = new TextProcessor();

  return function(text) {
    return textProcessor.ProcessText(text);
  };
})();
