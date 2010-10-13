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

  fetchArticle: function(context, guid, onSuccess, onFailure) {
    var self = this;
    var fetchId = this._lastFetchId = Math.random();

    $.ajax({
      url: 'article?id=' + context.id + '&guid=' + encodeURIComponent(guid),
      success: function(re) {
        self._handleAjaxSuccess(fetchId, onSuccess, onFailure, re);
      },
      error: function(xhr, ts, e) {
        self._handleAjaxFailure(fetchId, onFailure, xhr, ts, e);
      },
      cache: true
    });
  }
};

function ResultsContainer(context, response) {
  this._context = context;
  this._resultSet = response[context.name].results;
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
    for (var i = 0; i < this._resultSet.length; i++) {
      var tagString = this._resultSet[i][2];

      if (tagString === undefined) {
        continue;
      }

      var tags = tagString.split(',');
      for (var t = 0; t < tags.length; t++) {
        if (tags[t] === text) {
          this._offset = i;
          return true;
        }
      }
    }

    return false;
  },

  getResultCount: function() {
    return this._resultSet.length;
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

function SearchAgent() {
  this._lastSearchId = null;
}

SearchAgent.prototype = {
  _handleAjaxSuccess: function(ctx, searchId, response, onSuccess, onFailure) {
    if (searchId != this._lastSearchId) {
      return;
    }

    var error = response[ctx.name].error;
    if (error === undefined) {
      onSuccess(new ResultsContainer(ctx, response));
    } else {
      onFailure(error);
    }
  },

  _handleAjaxFailure: function(searchId, onFailure, xhr, textStatus, error) {
    if (searchId != this._lastSearchId) {
      return;
    }

    onFailure(error.toString());
  },

  search: function(context, query, onSuccess, onFailure) {
    var self = this;
    var searchId = this._lastSearchId = Math.random();

    $.ajax({
      url: 'search?id=' + context.id + '&q=' + encodeURIComponent(query),
      success: function(re) {
        self._handleAjaxSuccess(context, searchId, re, onSuccess, onFailure);
      },
      error: function(xhr, ts, e) {
        self._handleAjaxFailure(searchId, onFailure, xhr, ts, e);
      },
      cache: true
    });
  }
};

function MagicArticleAgent() {
  this._searchAgent = new SearchAgent();
  this._articleAgent = new ArticleAgent();
}

MagicArticleAgent.prototype = {
  _loadArticle: function(ctx, kw, results, onSuccess, onFailure) {
    if (results.seekMagic(kw)) {
      this._articleAgent.fetchArticle(
        ctx,
        results.getArticleGuid(),
        function(text) {
          onSuccess({
            results: results,
            text   : text
          });
        },
        function() {
          onSuccess({results: results});
        }
      );
    } else {
      onSuccess({results: results});
    }
  },

  getArticleAgent: function() {
    return this._articleAgent;
  },

  getSearchAgent: function() {
    return this._searchAgent;
  },

  fetchArticle: function(context, keyword, onSuccess, onFailure) {
    var self = this;

    this._searchAgent.search(
      context,
      keyword,
      function(results) {
        self._loadArticle(context, keyword, results, onSuccess, onFailure);
      },
      onFailure
    );
  }
};

(function($) {
  var self       = null;
  var eventTimer = null;
  var staleTicks = 0;
  var lastText   = '';

  function tryEmitEvent(force) {
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

  $.fn.startInstantFeedback = function() {
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

  $.fn.setText = function(text, silent) {
    $(this).val(text);

    if (silent) {
      lastText = text;
    } else {
      tryEmitEvent();
    }
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

function ArticleWidget(parent, conf) {
  this.__init__(parent, conf);
}

ArticleWidget.prototype = {
  __init__: function(parent, conf) {
    this.__proto__.__proto__.__init__.call(this);
    this._discardLookup = false;
    this._containerDom =
      $('<div class="' + (conf.containerClass || 'article') + '"></div>');
    $(parent).append(this._containerDom);

    $(this._containerDom).bind('mouseup', {self:this}, this._handleMouseUp);
    $(window).bind('keydown', {self:this}, this._handleKeyDown);
    $(window).bind('keyup', {self:this}, this._handleKeyUp);
  },

  _handleKeyDown: function(event) {
    if (event.keyCode == 17 || event.keyCode == 91) {
      event.data.self._discardLookup = true;
    }
  },

  _handleKeyUp: function(event) {
    if (event.keyCode == 17 || event.keyCode == 91) {
      event.data.self._discardLookup = false;
    }
  },

  _handleMouseUp: function(event) {
    var self = event.data.self;
    var text = document.getSelection().toString();

    if (text.length > 0 && !self._discardLookup) {
      self.trigger('inlineLookup', [text]);
    }
  },

  renderLoadingPage: function() {
    $(this._containerDom).html('');
  },

  renderArticle: function(text) {
    $(this._containerDom).html(text);
  },

  show: function() {
    $(this._containerDom).show();
  },

  hide: function() {
    $(this._containerDom).hide();
  },

  visible: function() {
    return $(this._containerDom).is(':visible');
  }
}; MergePrototype(ArticleWidget.prototype, SignalUserPrototype);

function SearchResultsWidget(parent, conf) {
  this.__init__(parent, conf);
}

SearchResultsWidget.prototype = {
  __init__: function(parent, conf) {
    this.__proto__.__proto__.__init__.call(this);
    this._containerDom =
      $('<div class="' + (conf.containerClass || 'results') + '"></div>');

    $(parent).append(this._containerDom);
    $(this._containerDom).bind('click', {self:this}, this._handleClick);
  },

  _handleClick: function(event) {
    var self = event.data.self;

    if ($(event.target).hasClass('result')) {
      event.preventDefault();
      self.trigger('openArticle', [$(event.target).attr('href')]);
    }
  },

  renderLoadingPage: function() {
    // TODO: Add 'Loading ...' page.
    $(this._containerDom).html('');
  },

  renderNoQueryPage: function() {
    // TODO: Add 'Empty query' page.
    $(this._containerDom).html('');
  },

  renderResultsPage: function(results) {
    results.seekFirst();

    var stringList = [];
    if (results.getResultCount() > 0) {
      stringList.push('<h3 class="result-count">Found ');
      stringList.push(results.getResultCount().toString());
      stringList.push(results.getResultCount() == 1 ? ' match' : ' matches');
      stringList.push('</h3>');

      while (results.seekNext()) {
        stringList.push('<a class="result" href="');
        stringList.push(results.getArticleGuid());
        stringList.push('">');
        stringList.push(results.getArticleHeading());
        stringList.push('</a>');
      }
    } else {
      stringList.push('<span class="result-count">No matches found</span>');
    }

    $(this._containerDom).html(stringList.join(''));
  },

  show: function() {
    $(this._containerDom).show();
  },

  hide: function() {
    $(this._containerDom).hide();
  },

  visible: function() {
    return $(this._containerDom).is(':visible');
  }
}; MergePrototype(SearchResultsWidget.prototype, SignalUserPrototype);

function OverlayArticleWidget(parent, conf) {
  // Dynamically create and append widget's DOM to parent's DOM.
  this._overlayDom = $(
    '<div class="' + (conf.overlayClass || 'overlay') + '"> \
       <div class="overlay-content"></div> \
       <div class="overlay-menu"> \
         <a href="" class="overlay-back-btn"></a> \
         <a href="" class="overlay-fwd-btn"></a> \
         <div class="overlay-tabs"> \
           <a href="" class="overlay-article-btn">Article</a> \
           <a href="" class="overlay-results-btn">Search</a> \
         </div> \
       </div> \
     </div>'
  );
  $(parent).append(this._overlayDom);

  // Create Article/Search tab panes.
  this._articleWidget =
    new ArticleWidget($('.overlay-content', this._overlayDom), {
      containerClass: 'overlay-article'
    });
  this._searchWidget =
    new SearchResultsWidget($('.overlay-content', this._overlayDom), {
      containerClass: 'overlay-results'
    });

  var self = this;
  // Bind handler that will open an article when user clicks on a search result.
  this._searchWidget.bind('openArticle', function(guid) {
    self._handleOpenArticleEvent(guid);
  });

  // Bind handler that will will load an article in response to inline lookup
  // request.
  this._articleWidget.bind('inlineLookup', function(text) {
    self._handleInlineLookupEvent(text);
  });

  // Bind back/forward button handlers.
  $('.overlay-back-btn', this._overlayDom).bind('click', function(event) {
    self._handleBackBtnClick(event);
  });
  $('.overlay-fwd-btn', this._overlayDom).bind('click', function(event) {
    self._handleFwdBtnClick(event);
  });

  // Initialize overlay.
  $(this._overlayDom).overlay();

  // Initialize tab widget.
  $('.overlay-tabs', this._overlayDom).tabs(
    $('.overlay-content > div', this._overlayDom), {
      effect: 'fade',
      initialIndex: null
    }
  );

  this._overlay = $(this._overlayDom).data('overlay');
  this._tabs = $('.overlay-tabs', this._overlayDom).data('tabs');
  this._constraintElement = conf.constraintElement || window;
  this._magicAgent = new MagicArticleAgent();

  this._history = [];
  this._current = 0;
}

OverlayArticleWidget.prototype = {
  _handleInlineLookupEvent: function(text) {
    this._load(this._history[this._current].dictContext, text, true);
  },

  _handleBackBtnClick: function(event) {
    event.preventDefault();
    this._select(this._current - 1);
  },

  _handleFwdBtnClick: function(event) {
    event.preventDefault();
    this._select(this._current + 1);
  },

  _handleOpenArticleEvent: function(guid) {
    var self = this;
    var current = this._current;
    var activeItem = this._history[current];
    var context = activeItem.dictContext;

    this._articleWidget.renderLoadingPage();
    this._magicAgent.getArticleAgent().fetchArticle(
      context,
      guid,
      function(text) {
        activeItem.articleText = text;

        // Do not render article if user navigated away from the current
        // history item.
        if (current == self._current) {
          self._articleWidget.renderArticle(text);
          self._switchToArticle();
        }
      },
      function(details) {
        console.error('OverlayArticleWidget: fetchArticle: GUID=' + guid);
      }
    );
  },

  _adjustGeometry: function() {
    var containerWidth = $(this._constraintElement).outerWidth();
    var containerHeight = $(this._constraintElement).outerHeight();
    var containerOffset = $(this._constraintElement).offset();

    $(this._overlayDom).width(containerWidth - 90);
    $(this._overlayDom).height(containerHeight - 60);
    this._overlay.getConf().top = containerOffset.top +
      (containerHeight - $(this._overlayDom).outerHeight()) / 2;
    this._overlay.getConf().left = containerOffset.left +
      (containerWidth - $(this._overlayDom).outerWidth()) / 2;
  },

  _renderOverlay: function() {
    if (this._overlay.isOpened()) {
      return;
    }

    this._adjustGeometry();

    var self = this;
    $(this._overlayDom).expose({
      loadSpeed: 400,
      color: 'rgba(0,0,0,.4)',
      onBeforeLoad: function() {
        self._overlay.load();
      },
      onBeforeClose: function() {
        self._overlay.close();
      }
    });
  },

  _processResults: function(dictContext, resultObject) {
    if (resultObject.results.getResultCount() == 0) {
      return;
    }

    var item = {
      dictContext  : dictContext,
      searchResults: resultObject.results,
      articleText  : resultObject.text
    };

    this._history.splice(this._current + 1);
    this._history.push(item);
    this._select(this._history.length - 1);
  },

  _switchToArticle: function() {
    this._tabs.click(
      $('.overlay-tabs > .overlay-article-btn', this._overlayDom).index());
  },

  _switchToResults: function() {
    this._tabs.click(
      $('.overlay-tabs > .overlay-results-btn', this._overlayDom).index());
  },

  _select: function(index) {
    var activeItem = this._history[index];

    if (activeItem !== undefined) {
      this._current = index;
    } else {
      return;
    }

    var searchResults = activeItem.searchResults;
    var articleText = activeItem.articleText;

    if (articleText !== undefined && searchResults !== undefined) {
      this._articleWidget.renderArticle(articleText);
      this._searchWidget.renderResultsPage(searchResults);
      this._switchToArticle();
    } else {
      this._searchWidget.renderResultsPage(searchResults);
      this._switchToResults();
    }

    if (index + 1 < this._history.length) {
      $('.overlay-fwd-btn', this._overlayDom).removeClass('inactive-pixmap');
    } else {
      $('.overlay-fwd-btn', this._overlayDom).addClass('inactive-pixmap');
    }

    if (index - 1 >= 0) {
      $('.overlay-back-btn', this._overlayDom).removeClass('inactive-pixmap');
    } else {
      $('.overlay-back-btn', this._overlayDom).addClass('inactive-pixmap');
    }

    $('.overlay-results-btn', this._overlayDom).html(
      'Search <span class="result-count">(' + activeItem.searchResults.getResultCount() + ')</span>');

    this._renderOverlay();
  },

  _load: function(context, query, preserveHistory) {
    var self = this;

    if (!preserveHistory) {
      this._history = [];
    }

    this._magicAgent.fetchArticle(
      context,
      query,
      function(resultObject) {
        self._processResults(context, resultObject);
      },
      function() {
        console.error('OverlayArticleWidget fetch failed');
      }
    );
  },

  show: function(context, query) {
    this._load(context, query, false);
  }
};
