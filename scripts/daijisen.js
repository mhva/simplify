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
  0xb424: '↔',
  0xb852: '⇨',
  0xc455: '(文)',
  0xa330: 'ū',
  0xa224: 'ā',
  0xa36d: 'ī'
};

var articleGaijiRanges = {
  lexicalCategory : new CharMapper([0xc437, 0x2160 /* Ⅰ */ , 6  /* VI */]),
  unknownCategory : new CharMapper([0xc431, 0x3220 /* ㈠ */, 5  /* ㈤ */]),
  meaningNumber   : new CharMapper([0xc373, 0x2460 /* ① */ , 12 /* ⑫ */],
                                   [0xc421, 0x246c /* ⑬ */ , 8  /* ⑳ */],
                                   [0xc429, 0x3251 /* ㉑ */, 5  /* ㉕ */],
                                   [0xc440, 0x3256 /* ㉖ */, 7  /* ㉝ */]),
  subMeaningNumber: new CharMapper([0xb646, 0x32d0 /* ㋐ */, 8  /* ㋗ */],
                                   [0xb322, 0x32d8 /* ㋘ */, 6  /* ㋛ */]),
  spec            : new CharMapper([0xb64e, 0x2488 /* ⒈ */ , 5])
};

// ====================================================================
// TextProcessor.
// ====================================================================

function TextProcessor() {}
TextProcessor.prototype = {
  /**
   * Escapes list numbers that are enclosed into certain brackets.
   * These list numbers are known to be references.
   *
   * Each such number occurrence is replaced with numeric character reference.
   */
  _EscapeEnclosedListNumbers: (function() {
    var matchListNumberRe =
      new RegExp('[' +
                 articleGaijiRanges.lexicalCategory.GetRegexpRange() +
                 articleGaijiRanges.unknownCategory.GetRegexpRange() +
                 articleGaijiRanges.meaningNumber.GetRegexpRange() +
                 articleGaijiRanges.subMeaningNumber.GetRegexpRange() +
                 ']');

    return function(text) {
      var chunks = [];

      var left = '（［「《〈';
      var right = '〉》」］）';

      var openCount = 0;
      var sliceFrom = 0;

      for (var i = 0; i < text.length; i++) {
        var lindex;
        var rindex;

        if (matchListNumberRe.test(text[i])) {
          if (openCount > 0) {
            chunks.push(text.slice(sliceFrom, i));
            chunks.push('&#' + text[i].charCodeAt(0) + ';');
          } else {
            chunks.push(text.slice(sliceFrom, i + 1));
          }

          sliceFrom = i + 1;
        } else if ((lindex = left.indexOf(text[i])) != -1) {
          openCount++;
        } else if ((rindex = right.indexOf(text[i])) != -1) {
          if (openCount > 0) {
            openCount--;
          } else {
            print('Mismatched parentheses in text at offset ' + i);
          }
        }
      }

      if (sliceFrom < text.length) {
        chunks.push(text.slice(sliceFrom));
      }

      if (openCount > 0) {
        print('Got some unclosed parentheses in text');
      }

      return chunks.join('');
    };
  })(),

  /**
   * Escapes list numbers that are references in disguise.
   *
   * The text representation of these references is indistinguishable from
   * either Lexical Category Numbers, or [Unknown] Category Numbers, or
   * Meaning Numbers. This function tries its best to escape these references.
   */
  _EscapeInTextReferences: (function() {
    var checkpointRe =
      new RegExp('[' +
                 articleGaijiRanges.lexicalCategory.GetRegexpRange() +
                 articleGaijiRanges.unknownCategory.GetRegexpRange() +
                 articleGaijiRanges.meaningNumber.GetRegexpRange() +
                 ']', 'g');

    var actors = [
      {
        mapper: articleGaijiRanges.meaningNumber,
        last  : null,
        reset : function() {}
      },
      {
        mapper: articleGaijiRanges.lexicalCategory,
        last  : null,
        reset : function() { actors[0].last = null; }
      },
      {
        mapper: articleGaijiRanges.unknownCategory,
        last  : null,
        reset : function() { actors[0].last = null; }
      }
    ];

    // A function that will be called on each matching list number and will
    // determine whether the passed list number is a reference or not.
    var replaceFun = function($0) {
      for (var i = 0; i < actors.length; i++) {
        var actor = actors[i];
        var number = actor.mapper.GetDstCharDelta($0) + 1;

        if (!isNaN(number)) {
          if ((actor.last === null && number == 1) || number == actor.last + 1){
            actor.last = number;
            actor.reset();
            return $0;
          } else {
            return '&#' + $0.charCodeAt(0) + ';';
          }
        }
      }

      print('BUG: Shouldn\'t be here. Match: ' + $0);
      return $0;
    };

    return function(text) {
      text = this._EscapeEnclosedListNumbers(text);

      // Reset each actor's state.
      for (var i = 0; i < actors.length; i++) {
        actors[i].last = null;
      }

      return text.replace(checkpointRe, replaceFun);
    };
  })(),

  /**
   * Escapes all list numbers in the given string. Each number occurrence will
   * be converted into HTML numberic character code.
   */
  _EscapeListNumbers: (function() {
    var matchListNumberRe =
      new RegExp('[' +
                 articleGaijiRanges.lexicalCategory.GetRegexpRange() +
                 articleGaijiRanges.unknownCategory.GetRegexpRange() +
                 articleGaijiRanges.meaningNumber.GetRegexpRange() +
                 articleGaijiRanges.subMeaningNumber.GetRegexpRange() +
                 ']', 'g');

    var replaceFun = function($0) {
      return '&#' + $0.charCodeAt(0) + ';';
    };

    return function(text) {
      return text.replace(matchListNumberRe, replaceFun);
    };
  })(),

  /**
   * Finds and encloses article's title into HTML tags.
   */
  _DecorateTitle: (function() {
    var matchTitleRe = /^\s*(.*?)␊\s*/;
    var replaceFun = function($0, $1) {
      return '<div class="article-title">' + $1 + '</div>';
    };

    return function(text) {
      return text.replace(matchTitleRe, replaceFun);
    };
  })(),

  /**
   * Finds and encloses category titles into HTML tags.
   */
  _DecorateCategories: (function() {
    var matchNumberedLexCatRe =
      new RegExp('[' +
                 articleGaijiRanges.lexicalCategory.GetRegexpRange() +
                 '][^␊◆' +
                 articleGaijiRanges.lexicalCategory.GetRegexpRange() +
                 articleGaijiRanges.meaningNumber.GetDstCharWithIndex(0) +
                ']*', 'g');

    var matchNumberedUnkCatRe =
      new RegExp('[' +
                 articleGaijiRanges.unknownCategory.GetRegexpRange() +
                 '][^␊◆' +
                 articleGaijiRanges.unknownCategory.GetRegexpRange() +
                 articleGaijiRanges.meaningNumber.GetDstCharWithIndex(0) +
                 ']*', 'g');

    var matchSingularLexCatRe =
      new RegExp('(^.*?␊)(.*?)([' +
                 articleGaijiRanges.meaningNumber.GetDstCharWithIndex(0) +
                 '])');

    return function(text) {
      // Replace [Unknown] Category items.
      text = text.replace(matchNumberedUnkCatRe, function($0) {
        return '␊<div class="djs-section">' + $0 + '</div>';
      });

      // Replace numbered Lexical Category items.
      var hasMatches = false;

      text = text.replace(matchNumberedLexCatRe, function($0) {
        hasMatches = true;
        return '␊<div class="djs-section">' + $0 + '</div>';
      });

      // Finish if we have found numbered list items. None of other section
      // forms are possible in this text.
      if (hasMatches) {
        return text;
      }

      // Try to find one and only one unnumbered item. Daijisen doesn't number
      // sections if there is only one section.
      text = text.replace(matchSingularLexCatRe, function($0, $1, $2, $3) {
        hasMatches = true;
        return $1 + '<div class="djs-section">' + $2 + '</div>' + $3;
      });

      return text;
    };
  })(),

  /**
   * Helper function for finding and enclosing sub-meanings into HTML tags.
   *
   * The string must be a text of parent meaning.
   */
  _DecorateSubMeanings: (function() {
    var matchSubMeaningRe =
      new RegExp('([' +
                 articleGaijiRanges.subMeaningNumber.GetRegexpRange() +
                 '])([^' +
                 articleGaijiRanges.subMeaningNumber.GetRegexpRange() +
                 ']+)', 'g');

    var replaceFun = function($0, $1, $2) {
      return '<div class="a-li"><span class="a-ln">' + $1 +
        '</span><span class="a-lt">' + $2 + '</span></div>';
    };

    return function(meaningText) {
      return meaningText.replace(matchSubMeaningRe, replaceFun);
    };
  })(),

  /**
   * Finds and encloses meanings and sub-meanings into HTML tags.
   */
  _DecorateMeanings: (function() {
    var matchMeaningRe =
      new RegExp('([' +
                 articleGaijiRanges.meaningNumber.GetRegexpRange() +
                '])([^␊◆' +
                 articleGaijiRanges.meaningNumber.GetRegexpRange() +
                ']+)', 'g');

    var mapper = articleGaijiRanges.meaningNumber;

    return function(text) {
      var self = this;

      return text.replace(matchMeaningRe, function($0, $1, $2) {
        var number = mapper.GetDstCharDelta($1) + 1;

        return '<div class="a-li"><span class="a-ln">' +
          number + '</span><span class="a-lt">' +
          self._DecorateSubMeanings($2) + '</span></div>';
      });
    };
  })(),

  /**
   * Finds and encloses special sections(e.g. 類語,下接句 ...) into HTML tags.
   */
  _DecorateSpecialSections: (function() {
    var matchSpecialSectionRe =
      new RegExp('␊［(下接句|下接語|可能|類語|用法|派生|形動)］([^◆␊' +
                 articleGaijiRanges.lexicalCategory.GetRegexpRange() +
                 articleGaijiRanges.unknownCategory.GetRegexpRange() +
                ']*)', 'g');

    var matchRuigoSectionRe =
      new RegExp('◆([^␊◆' +
                 articleGaijiRanges.lexicalCategory.GetRegexpRange() +
                 articleGaijiRanges.unknownCategory.GetRegexpRange() +
                ']*)', 'g');

    var Decorate = function(name, text) {
      return '␊<div class="a-spec-item"><span class="a-spec-name"><p>' + name +
        '</p></span><span class="a-spec-text">' +
        this._EscapeListNumbers(text) + '</span></div>';
    };

    return function(text) {
      var self = this;

      text = text.replace(matchSpecialSectionRe, function($0, $1, $2) {
        return Decorate.call(self, $1, $2);
      });

      text = text.replace(matchRuigoSectionRe, function($0, $1) {
        return Decorate.call(self, '補説', $1);
      });

      return text;
    };
  })(),

  /**
   * Strips garbage characters that we were using as stop chars or something
   * else unworthy from string.
   *
   * Should be used after the main processing is done.
   */
  _StripGarbage: (function() {
    var matchGarbageCharsRe = /␊/g;

    return function(text) {
      return text.replace(matchGarbageCharsRe, '');
    };
  })(),

  ProcessText: function(text) {
    text = this._EscapeInTextReferences(text);
    text = this._DecorateSpecialSections(text);
    text = this._DecorateCategories(text);
    text = this._DecorateMeanings(text);
    text = this._DecorateTitle(text);
    text = this._StripGarbage(text);

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
   * Strips some characters from the heading. These characters are used to
   * show which writings are old Japanese or deprecated, separate hiragana
   * readings, etc...
   *
   * The problem with these characters that they clutter search results too
   * much. These characters can still be seen in the article page.
   */
  _StripGarbageChars: (function() {
    var Strip = function(text, startAt, inferior, stopOn, out) {
      var i = startAt;
      var sliceFrom = startAt;

      for (; i < text.length && text[i] != stopOn; i++) {
        if (inferior.indexOf(text[i]) == -1) {
          // An empty branch to aid branch predicition mechanism (hopefully).
        } else {
          out.push(text.slice(sliceFrom, i));
          sliceFrom = i + 1;
        }
      }

      out.push(text.slice(sliceFrom, i + 1));
      return i;
    };

    return function(text) {
      var chunks = [];
      var stoppedAt = Strip(text, 0, '‐・', '【', chunks);

      // Sanitiaze the so called 'Kanji Part' (text enclosed into 【】),
      // if any.
      if (stoppedAt < text.length) {
        stoppedAt = Strip(text, stoppedAt + 1, '‐＝△×', '】', chunks);

        // Ensure that we have parsed the whole string.
        if (stoppedAt + 1 >= text.length) {
          return chunks.join('');
        } else {
          print('BUG: Unexpected string continuation in: ' + text + '. ' +
                'Did you join kanji parts?');
          return text;
        }
      } else {
        return chunks.join('');
      }
    };
  })(),

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
    var matchHiraganaKatakanaRe = /[あ-ヺ]/;

    return function(heading) {
      // Headings containing one character are often represent an entry
      // describing Kanji. These entries are not very useful so we drop them.
      if (heading.length > 1) {
        heading = this._JoinKanjiParts(heading);
        heading = this._StripGarbageChars(heading);

        // HACK: Save the result so the ProcessTags can later pick it up and
        // avoid burning unneccessary CPU cycles.
        this.processedHeading = heading;

        heading = this._TranslateFullWidthLatin(heading);
        return _EscapeJsonString(heading);
      } else {
        // Be 100% that we are skipping a Kanji entry. There're some headings
        // containing only 1 hiragana/katakana char and we don't want to drop
        // those.
        if (matchHiraganaKatakanaRe.test(heading)) {
          this.processedHeading = heading;
          return heading;
        } else {
          this.processedHeading = null;
          return null;
        }
      }
    };
  })(),

  ProcessTags: function(heading) {
    var templSource;

    if (this.processedHeading !== undefined) {
      templSource = this.processedHeading;
      this.processedHeading = undefined;
    } else {
      // TODO: Might be a good idea to make ProcessHeading() to look for cached
      // result.
      print('Entering a slow processing path in ProcessTags function. Please ' +
            'consider calling Dictionary::SearchResults::FetchHeading() ' +
            'before calling Dictionary::SearchResults::FetchTags().');
      templSource = this.ProcessHeading();
    }

    if (templSource !== null) {
      var left;

      // If we have some templates- permute them and produce tags.
      if (templSource[templSource.length - 1] == '】' &&
          (left = templSource.indexOf('【')) != -1) {
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
        // The heading contains only a reading, so we make this reading a tag.

        // Some readings might contain Kanji with their readings enclosed in
        // full width parentheses (WTF, this is a reading for God's sake).
        // We should strip these readings to produce valid tags.
        return this._StripChunksEnclosedInParentheses(heading);
      }
    } else {
      return '';
    }
  }
};

// ====================================================================
// Hooks.
// ====================================================================

function Newline() {
  return '␊';
}

function Indent() {
  return '';
}

function InsertHeadingGaiji(gaijiCode) {
  var result = gaijiMap[gaijiCode];
  return (result) ? result : '?';
}

function InsertTextGaiji(gaijiCode) {
  var result;

  // Try to look up the gaiji in the gaiji ranges map.
  for (var key in articleGaijiRanges) {
    result = articleGaijiRanges[key].MapToChar(gaijiCode);

    if (result !== undefined) {
      return result;
    }
  }

  // Try to look up the gaiji in the map with singular gaiji.
  result = gaijiMap[gaijiCode];
  return (result) ? result : '&lt;0x' + gaijiCode.toString(16) + '&gt;';
}

function BeginKeyword() {
  // Keywords in DAIJISEN appear to depict article's title.
  return '';
}

function EndKeyword() {
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
  }
})();
