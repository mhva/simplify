/**
 * Takes 2 or more objects and extends the object in first argument
 * with properties of other objects. This function doesn't overwrite
 * existing properties.
 *
 * This function is used to extend prototypes, hence the name.
 *
 * @return Returns extended object (IOW, object in the first argument).
 */
function MergePrototype(proto) {
  for (var i = 1; i < arguments.length; i++) {
    var other = arguments[i];

    for (var key in other) {
      if (!(key in proto) && other.hasOwnProperty(key)) {
        proto[key] = other[key];
      }
    }
  }

  return proto;
}

/**
 * ArticleAgent provides means of fetching dictionary articles.
 */
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

  /**
   * Fetches an article from specified dictionary.
   *
   * @param context   Dictionary context of the target dictionary.
   * @param guid      Article's GUID.
   * @param onSuccess Callback function that will be called if fetch succeeds.
   *  This function receives one argument containing article's text.
   * @param onFailure Callback function that will be called if fetch fails.
   *  This function receives one argument containing an error message.
   *
   * @note If this method is called before previous fetch is complete,
   *  results of the previous fetch will not be delivered.
   */
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

/**
 * A container class for search results.
 *
 * @param context  Context of the dictionary to which the search results belong.
 * @param response Server response.
 */
function ResultsContainer(context, response) {
  this._context = context;
  this._resultSet = response[context.name].results;
  this._offset = -1;
}

ResultsContainer.prototype = {
  _getElement: function(elementIndex) {
    var result = this._resultSet[this._offset];

    if (result !== undefined) {
      return result[elementIndex];
    } else {
      return undefined;
    }
  },

  /**
   * Puts the seek pointer into its initial position.
   */
  seekFirst: function() {
    this._offset = -1;
  },

  /**
   * Seeks next result.
   *
   * Example usage:
   *   results.seekFirst();
   *   while (results.seekNext()) {
   *     //process result//
   *   }
   *
   * @return Returns true if seek succeeded; otherwise false.
   */
  seekNext: function() {
    if (this._offset + 1 < this._resultSet.length) {
      this._offset++;
      return true;
    } else {
      return false;
    }
  },

  /**
   * Tries to seek a result that has a tag @tagName.
   *
   * @return Returns true if seek succeeded; otherwise false.
   */
  seekMagic: function(tagName) {
    for (var i = 0; i < this._resultSet.length; i++) {
      var tagString = this._resultSet[i][2];

      if (tagString === undefined) {
        continue;
      }

      var tags = tagString.split(',');
      for (var t = 0; t < tags.length; t++) {
        if (tags[t] === tagName) {
          this._offset = i;
          return true;
        }
      }
    }

    return false;
  },

  /**
   * Returns the number of search results.
   */
  getResultCount: function() {
    return this._resultSet.length;
  },

  /**
   * Returns article's guid at the current position.
   *
   * @note Calling this method before calling the @seekNext() method, or
   *  calling it after the @seekNext() method returned false, results in
   *  undefined behavior.
   */
  getArticleGuid: function() {
    return this._getElement(0);
  },

  /**
   * Returns article's heading at the current position.
   *
   * @note Calling this method before calling the @seekNext() method, or
   *  calling it after the @seekNext() method returned false, results in
   *  undefined behavior.
   */
  getArticleHeading: function() {
    return this._getElement(1);
  },

  /**
   * Returns article's tags at the current position.
   *
   * @note Calling this method before calling the @seekNext() method, or
   *  calling it after the @seekNext() method returned false, results in
   *  undefined behavior.
   */
  getArticleTags: function() {
    return this._getElement(2) || '';
  },

  /**
   * Checks whether the search results were truncated.
   *
   * Results are truncated if dictionary has returned too many results for
   * specific query. Currently, there is no way to retrieve other results
   * other than using a more specific search query.
   */
  isTruncated: function() {
    // TODO: Not implemented.
    console.warning('ResultsContainer.isTruncated() is not implemented');
    return false;
  }
};

/**
 * SearchAgent provides means of searching a dictionary.
 */
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

  /**
   * Searches specified dictionary.
   *
   * @param context   Dictionary context of the target dictionary.
   * @param query     Search query.
   * @param onSuccess Callback function that will be called if search succeeds.
   *  This function receives one argument containing a ResultsContainer object.
   * @param onFailure Callback function that will be called if search fails.
   *  This function receives one argument containing an error message.
   *
   * @note If this method is called before previous search is complete,
   *  results of the previous search will not be delivered.
   */
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

/**
 * MagicArticleAgent provides a means to fetch an article by tag.
 */
function MagicArticleAgent() {
  this._searchAgent = new SearchAgent();
  this._articleAgent = new ArticleAgent();
}

MagicArticleAgent.prototype = {
  _loadArticle: function(ctx, tagName, results, onSuccess, onFailure) {
    if (results.seekMagic(tagName)) {
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

  /**
   * Returns an ArticleAgent object used by the MagicArticleAgent to
   * fetch articles.
   *
   * @note Using this agent while the MagicArticleAgent doing its work
   *  might result in results being not delivered.
   */
  getArticleAgent: function() {
    return this._articleAgent;
  },

  /**
   * Returns an ArticleAgent object used by the MagicArticleAgent to
   * do search.
   *
   * @note Using this agent while the MagicArticleAgent doing its work
   *  might result in results being not delivered.
   */
  getSearchAgent: function() {
    return this._searchAgent;
  },

  /**
   * Fetches an article by tag from specified dictionary.
   *
   * @param context Dictionary context of the target dictionary.
   * @param tagName Tag name.
   * @param onSuccess Callback function that will be called if fetch succeeds.
   *  This function receives one argument containing a dictionary with the
   *  following keys: searchResults, articleText. The articleText might not
   *  exist in the dictionary if article with specified tag was not found.
   * @param onFailure Callback function that will be called if fetch fails.
   *  This function receives one argument containing an error message.
   */
  fetchArticle: function(context, tagName, onSuccess, onFailure) {
    var self = this;

    this._searchAgent.search(
      context,
      tagName,
      function(results) {
        self._loadArticle(context, tagName, results, onSuccess, onFailure);
      },
      onFailure
    );
  }
};

/**
 * jQuery plugin that makes a text field do a feedback when text in the field
 * is changed.
 *
 * Once the plugin is initialized on text field it will emit the 'textchanged'
 * signal every time the text in the field is changed.
 */
(function($) {
  var self       = null;
  var eventTimer = null;
  var staleTicks = 0;
  var lastText   = '';

  function tryEmitEvent(force) {
    var text = $(self).val();
    if (text != lastText || force) {
      // Reset the stale period to indicate that user is still typing.
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

  /**
   * Initializes plugin.
   */
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

  /**
   * Sets value of the text field.
   *
   * If silent is true, 'textchanged' signal will not be emitted.
   */
  $.fn.setText = function(text, silent) {
    $(this).val(text);

    if (silent) {
      lastText = text;
    } else {
      tryEmitEvent();
    }
  };
})(jQuery);

/**
 * EventSource is an object that can be used to extend any object, giving
 * the object an ability to emit signals and others to be able to subscribe to
 * these signals.
 */
function EventSource() {
  this.__init__();
}

EventSource.prototype = {
  __init__: function() {
    this._listeners = {};
  },

  /**
   * Subscribes callback function to receive notifications when signal is
   * emitted.
   *
   * @param signalName The name of the signal.
   * @param listener   Function that should be invoked every time the signal
   *  is emitted.
   */
  bind: function(signalName, listener) {
    if (this._listeners[signalName] === undefined) {
      this._listeners[signalName] = [listener];
    } else {
      this._listeners[signalName].push(listener);
    }
  },

  /**
   * Unsubscribes specified function from receiving signal notifications.
   *
   * @param signalName The name of the signal.
   * @param listener   Function that should be unsubscribed from receiving
   *  notifications. If this parameter is omitted, all subscribers will be
   *  unsubscribed from receiving the signal.
   */
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

  /**
   * Triggers a signal.
   *
   * @param signalName The name of the signal.
   * @param argv       A list of arguments to pass to each callback function.
   */
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

ArticleWidget.prototype = MergePrototype({
  __init__: function(parent, conf) {
    EventSource.prototype.__init__.call(this);

    this._discardLookup = false;
    this._containerDom =
      $('<div class="' + (conf.containerClass || 'article') + '"></div>');
    $(parent).append(this._containerDom);

    $(this._containerDom).bind('click', {self:this}, this._handleClick);
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

  _handleClick: function(event) {
    if ($(event.target).hasClass('a-ref')) {
      event.preventDefault();
      event.data.self.trigger('replaceArticle', [$(event.target).attr('href')]);
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
}, EventSource.prototype);

function SearchResultsWidget(parent, conf) {
  this.__init__(parent, conf);
}

SearchResultsWidget.prototype = MergePrototype({
  __init__: function(parent, conf) {
    EventSource.prototype.__init__.call(this);

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
      stringList.push('<h3 class="result-count">No matches found</h3>');
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
}, EventSource.prototype);

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
