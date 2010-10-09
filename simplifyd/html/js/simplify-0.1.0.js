function MergePrototype(proto) {
  var tail = proto;

  for (var i = 1; i < arguments.length; i++) {
    var classProto = arguments[i];

    for (var key in classProto) {
      if (!(key in proto)) {
        proto[key] = classProto[key];
      }
    }

    tail.__proto__ = classProto;
    tail = classProto;
  }

  return proto;
}

function MakePrototype() {
  var argv = [{}];

  for (var i = 0; i < arguments.length; i++) {
    argv.push(arguments[i]);
  }

  return MergePrototype.apply(this, argv);
}

function ArticleAgent(context) {
  this._context = context;
  this._lastFetchId = null;
}

ArticleAgent.prototype = {
  _handleAjaxSuccess: function(fetchId, onSuccess, onFailure, response) {
    if (this._lastFetchId == fetchId) {
      onSuccess(response.article);
    }
  },

  _handleAjaxFailure: function(fetchId, onFailure, xhr, statusText, error) {
    if (this._lastFetchId == fetchId) {
      onFailure(error.toString());
    }
  },

  fetchArticle: function(guid, onSuccess, onFailure) {
    var self = this;
    this._lastFetchId = Math.random();

    $.ajax({
      url: 'article?id=' + this._context.id + '&guid=' +
        encodeURIComponent(guid),
      success: function(response) {
        self._handleAjaxSuccess.call(self,
                                     self._lastFetchId,
                                     onSuccess,
                                     onFailure,
                                     response);
      },
      error: function(xhr, textStatus, error) {
        self._handleAjaxFailure.call(self,
                                     self._lastFetchId,
                                     onFailure,
                                     xhr,
                                     textStatus,
                                     error);
      },
      cache: true
    });
  }
};

function ResultsContainer(dictContext, responseContext) {
  this._context = dictContext;
  this._responseContext = responseContext;
  this._resultSet = responseContext.results;
  this._offset = -1;
}

ResultsContainer.prototype = {
  seekFirst: function() {
    this._offset = -1;
  },

  seekNext: function() {
    if (this._offset + 1 < this._resultSet.length) {
      this._offset++;
      return true;
    } else {
      return false;
    }
  },

  seekMagic: function(text) {
    return false;
  },

  getArticleGuid: function() {
    return this._getElement(0);
  },

  getArticleHeading: function() {
    return this._getElement(1);
  },

  getArticleTags: function() {
    return this._getElement(2);
  },

  isTruncated: function() {
    // TODO:
    return false;
  },

  _getElement: function(elementIndex) {
    var result = this._resultSet[this._offset];

    if (result !== undefined) {
      return result[elementIndex];
    } else {
      return undefined;
    }
  }
};

function SearchAgent(context) {
  this._context = context;
  this._lastSearchId = null;
}

SearchAgent.prototype = {
  _handleAjaxSuccess: function(searchId, response, onSuccess, onFailure) {
    if (searchId != this._lastSearchId) {
      return;
    }

    if (!('error' in response)) {
      onSuccess(
        new ResultsContainer(this._context, response[this._context.name]));
    } else {
      onFailure(response.error);
    }
  },

  _handleAjaxFailure: function(searchId, onFailure, xhr, textStatus, error) {
    if (searchId != this._lastSearchId) {
      return;
    }

    onFailure(error.toString());
  },

  search: function(query, onSuccess, onFailure) {
    var self = this;
    this._lastSearchId = Math.random();

    $.ajax({
      url: 'search?id=' + this._context.id + '&q=' + encodeURIComponent(query),
      success: function(response) {
        self._handleAjaxSuccess.call(self,
                                     self._lastSearchId,
                                     response,
                                     onSuccess,
                                     onFailure);
      },
      error: function(xhr, textStatus, error) {
        self._handleAjaxFailure.call(self,
                                     self._lastSearchId,
                                     onFailure,
                                     xhr,
                                     textStatus,
                                     error);
      },
      cache: true
    });
  }
};

// jQuery plugin that provides a quick way to make a text field
// to do an AJAX feedback every time user types text.
//
// This plugin is used by simplify to implement instant search.
//
// Usage:
//  $('#text-input').instantSearch('api/search?q=',
//                                 function() { console.log('success'); },
//                                 function() { console.log('failure'); });
(function($) {
  var self       = null;
  var eventTimer = null;
  var staleTicks = 0;
  var lastText   = '';

  var tryEmitEvent = function(force) {
    var text = $(self).val();
    if (text != lastText || force) {
      // Reset the stale period to indicate that the user is still
      // typing.
      staleTicks = 0;
      lastText = text;

      $(self).trigger('textchanged', [text]);
    } else {
      staleTicks++;

      // Kill the timer if user didn't type anything for some time.
      if (staleTicks >= 20) {
        clearInterval(eventTimer);
        eventTimer = null;
      }
    }
  };

  $.fn.doTextChangedFeedback = function() {
    self = this;

    $(this).bind('keydown', function(event) {
      if (event.keyCode == 13) {
        tryEmitEvent(true);
        event.preventDefault();
        return;
      }

      if (eventTimer === null) {
        eventTimer = setInterval(tryEmitEvent, 200);
      }
    });
  };
})(jQuery);

(function($) {
  var self = null;

  var calculateHeight = function() {
    return $(window).height() - $(self).offset().top;
  };

  $.fn.makeContainer = function() {
    self = this;

    $(self).height(calculateHeight);
    $(window).resize(function() { $(self).height(calculateHeight); });
  };
})(jQuery);

var SignalUserPrototype = {
  __init__: function() {
    this._listeners = {};
  },

  bind: function(signalName, listener) {
    if (this._listeners[signalName] === undefined) {
      this._listeners[signalName] = [listener];
    } else {
      this._listeners[signalName].push(listener);
    }
  },

  unbind: function(signalName, listener) {
    var listenerList = this._listeners[signalName];

    if (listenerList === undefined) {
      return;
    }

    if (listener !== undefined) {
      for (var i = 0; i < listenerList.length; i++) {
        if (listenerList[i] == listener) {
          listenerList.splice(i, 1);
          break;
        }
      }
    } else {
      delete this._listeners[signalName];
    }
  },

  trigger: function(signalName, argv) {
    var listenerList = this._listeners[signalName];

    if (listenerList !== undefined) {
      for (var i = 0; i < listenerList.length; i++) {
        listenerList[i].apply(this, argv);
      }
    }
  }
};
