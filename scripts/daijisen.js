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
  this.ranges = new Array();

  for (var i = 0; i < arguments.length; i++) {
    // TODO: Ensure that the range is correct.
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

      if (charCode >= range[0] && charCode < range[0] + range[2])
        return range[1] + (charCode - range[0]);
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

      if (index < delta + range[2])
        return String.fromCharCode(range[1] + (index - delta));
      else
        delta += range[2];
    }

    return undefined;
  }
}

var gaijiRangeMap = {
  lv0 : new CharMapper([0xc437, 0x2160 /* Ⅰ */ , 6 /* VI */]),
  lv05: new CharMapper([0xc431, 0x3220 /* ㈠ */, 5  /* ㈤ */]),
  lv1 : new CharMapper([0xc373, 0x2460 /* ① */ , 12 /* ⑫ */],
                       [0xc421, 0x246c /* ⑬ */ , 8  /* ⑳ */],
                       [0xc429, 0x3251 /* ㉑ */, 5  /* ㉕ */],
                       [0xc440, 0x3256 /* */, 7, /* */]),
  lv2 : new CharMapper([0xb646, 0x32d0 /* ㋐ */, 8  /* ㋗ */],
                       [0xb322, 0x32d8 /* ㋘ */, 6 /* ㋛ */]),
  spec: new CharMapper([0xb64e, 0x2488, 2])
}

var gaijiMap = {
  0xb424: '↔',
  0xb852: '⇨',
  0xc455: '(文)'
}

function Newline() {
  return '␊';
}

function Indent() {
  return '';
}

var matchHiraganaKatakana = /[あ-ヺ]/;
function ProcessHeading(text) {
  if (text.length > 1) {
    return text;
  } else {
    if (matchHiraganaKatakana.test(text)) {
      return text;
    } else {
      return null;
    }
  }
}

function InsertHeadingGaiji(gaijiCode) {
  return '?';
}

function InsertTextGaiji(gaijiCode) {
  // Try to look up the gaiji in the gaiji ranges map.
  for (key in gaijiRangeMap) {
    var result = gaijiRangeMap[key].MapToChar(gaijiCode);

    if (result !== undefined)
      return result;
  }

  // Try to look up the gaiji in the map with singular gaiji.
  var result = gaijiMap[gaijiCode];

  return (result) ? result : '&lt;0x' + gaijiCode.toString(16) + '&gt;';
}

var lv0Charset  = gaijiRangeMap.lv0.GetRegexpRange();
var lv05Charset = gaijiRangeMap.lv05.GetRegexpRange();
var lv1Charset  = gaijiRangeMap.lv1.GetRegexpRange();
var lv2Charset  = gaijiRangeMap.lv2.GetRegexpRange();

/**
 * Functions for decorating article title.
 */
var matchTitle = /^\s*(.*?)␊\s*/;
function DecorateTitle(text) {
  return text.replace(matchTitle,
                      '<div class="article-title djs-title">$1</div>');
}

/**
 * Functions for modifying relative references in the article's text.
 *
 * The text representation of these references is indistinguishable from
 * the Level 1 List Item numbers. DecorateRelativeReferences() function tries
 * its best to convert these references into plain numbers so that list
 * decorators can produce good results.
 */
var matchAnyRef = new RegExp('[' + lv0Charset + lv05Charset + lv1Charset + lv2Charset + ']');
function DecorateInnerRelativeReferences(text) {
  var result = [];

  var left = '（［「《〈';
  var right = '〉》」］）';

  var openParentheses = 0;
  var sliceFrom = 0;

  for (var i = 0; i < text.length; i++) {
    var lindex;
    var rindex;

    if (matchAnyRef.test(text[i])) {
      if (openParentheses > 0) {
        result.push(text.slice(sliceFrom, i));
        result.push('&#' + text[i].charCodeAt(0) + ';');
      } else {
        result.push(text.slice(sliceFrom, i + 1));
      }

      sliceFrom = i + 1;
    } else if ((lindex = left.indexOf(text[i])) != -1) {
      openParentheses++;
    } else if ((rindex = right.indexOf(text[i])) != -1) {
      if (openParentheses > 0) {
        openParentheses--;
      } else {
        print('Mismatched parentheses in text at offset ' + i);
      }
    }
  }

  if (sliceFrom < text.length)
    result.push(text.slice(sliceFrom));

  if (openParentheses > 0)
    print('Got some unclosed parentheses in text');

  return result.join('');
}

var matchRefCheckpoint = new RegExp('[' + lv0Charset + lv05Charset + lv1Charset + ']', 'g');
function DecorateRelativeReferences(text) {
  text = DecorateInnerRelativeReferences(text);

  var lv0mapper = gaijiRangeMap.lv0;
  var lv05mapper = gaijiRangeMap.lv05;
  var lv1mapper = gaijiRangeMap.lv1;

  var lastListNumber = null;
  var lastLv0Number = null;
  var lastLv05Number = null;

  return text.replace(matchRefCheckpoint, function($0) {
    var lv0Number;
    var lv05Number;
    var lv1Number;

    if ((lv1Number = lv1mapper.GetDstCharDelta($0)) !== undefined) {
      // Got a list item number from List 1.
      lv1Number++;

      if ((lastListNumber == null && lv1Number == 1)
          || lv1Number == lastListNumber + 1) {
        //print('item on ' + $0);
        lastListNumber = lv1Number;
        return $0;
      } else {
        return '&#' + $0.charCodeAt(0) + ';';
      }
    } else if ((lv0Number = lv0mapper.GetDstCharDelta($0)) !== undefined) {
      // Got a list item number from List 0.
      lv0Number++;

      if (lastLv0Number == null || lv0Number > lastLv0Number) {
        //print('item on ' + $0);
        lastListNumber = null;
        lastLv0Number = lv0Number;
        return $0;
      } else {
        return '&#' + $0.charCodeAt(0) + ';';
      }
    } else if ((lv05Number = lv05mapper.GetDstCharDelta($0)) !== undefined) {
      // Got a list item number from List 0.5.
      lv05Number++;

      if (lastLv05Number == null || lv05Number > lastLv05Number) {
        lastListNumber = null;
        lastLv05Number = lv05Number;
        return $0;
      } else {
        return '&#' + $0.charCodeAt(0) + ';';
      }
    } else {
      // WTF
      print('BUG: Invalid regexp or bug in mapper. Match: ' + $0);
      return $0;
    }
  })
}

function EscapeRelativeReferences(text) {
  return text.replace(matchAnyRef, function($0) {
    return '&#' + $0.charCodeAt(0) + ';';
  });
}

/**
 * Functions for decorating Level 0 List Items.
 */
var matchLv0NumericItem = new RegExp('[' + lv0Charset + '][^␊◆' + lv0Charset + gaijiRangeMap.lv1.GetDstCharWithIndex(0) + ']*', 'g');
var matchLv05NumericItem = new RegExp('[' + lv05Charset + '][^␊◆' + lv05Charset + gaijiRangeMap.lv1.GetDstCharWithIndex(0) + ']*', 'g');
var matchLv0SingularItem = new RegExp('(^.*?␊)(.*?)([' + gaijiRangeMap.lv1.GetDstCharWithIndex(0) + '])');
function DecorateLv0Items(text) {

  text = text.replace(matchLv05NumericItem, function($0) {
    return '␊<div class="djs-section">' + $0 + '</div>';
  });

  var hasMatches = false;

  // Try the numbered Level 0 List items.
  text = text.replace(matchLv0NumericItem, function($0) {
    hasMatches = true;
    return '␊<div class="djs-section">' + $0 + '</div>';
  });

  // Finish if we have found numbered list items. None of other section formats
  // are possible in the text anymore.
  if (hasMatches)
    return text;

  // Try to find one and only one unnumbered item. Daijisen doesn't number
  // sections if there is only one section.
  text = text.replace(matchLv0SingularItem, function($0, $1, $2, $3) {
    hasMatches = true;
    return $1 + '<div class="djs-section">' + $2 + '</div>' + $3;
  });

  return text;
}

/**
 * Functions for decorating Level 2 List Items.
 */
var matchLv2Item = new RegExp('([' + lv2Charset + '])([^' + lv2Charset +']+)', 'g');
function ReplaceLv2ItemText($0, $1, $2) {
  return '<div class="djs-lv2-item"><span class="djs-lv2-number">' + $1 +
    '</span><span class="djs-lv2-text">' + $2 + '</span></div>';
}

function DecorateLv2Items(lv1ItemText) {
  return lv1ItemText.replace(matchLv2Item, ReplaceLv2ItemText);
}

/**
 * Functions for decorating list items (0, 1, 2).
 */
var matchLv1Item = new RegExp('([' + lv1Charset + '])([^␊◆' + lv1Charset + lv0Charset + ']+)', 'g');
function DecorateListItems(text) {
  var lv1mapper = gaijiRangeMap.lv1;
  text = DecorateLv0Items(text);
  return text.replace(matchLv1Item, function($0, $1, $2) {
    return '<div class="djs-lv1-item"><span class="djs-lv1-number">' +
      (lv1mapper.GetDstCharDelta($1) + 1) +
      '</span><span class="djs-lv1-text">' + DecorateLv2Items($2) +
      '</span></div>';
  });
}

/**
 * Functions for decorating special sections.
 */
var matchSpecialSection = new RegExp('␊［(下接句|下接語|可能|類語|用法|派生|形動)］([^◆␊' + lv0Charset + lv05Charset + ']*)', 'g');
var matchRuigoSection = new RegExp('◆([^␊◆' + lv0Charset + lv05Charset + ']*)', 'g');
function DecorateSpecialSections(text) {
  text = text.replace(matchSpecialSection, function($0, $1, $2) {
    return '␊<div class="djs-spec"><span class="djs-spec-type"><p>' + $1 +
      '</p></span><span class="djs-spec-text">' +
      EscapeRelativeReferences($2) + '</span></div>';
  });
  text = text.replace(matchRuigoSection, function($0, $1) {
    return '␊<div class="djs-spec"><span class="djs-spec-type"><p>補説</p>' +
      '</span><span class="djs-spec-text">' + EscapeRelativeReferences($1) +
      '</span></div>';
  });
  return text;
}

var matchGarbageChars = /␊/g;
function ProcessText(text) {
  text = DecorateRelativeReferences(text);
  text = DecorateSpecialSections(text);
  text = DecorateListItems(text);
  text = DecorateTitle(text);
  text = text.replace(matchGarbageChars, '');

  return text;
}
