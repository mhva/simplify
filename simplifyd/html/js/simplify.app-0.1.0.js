"use strict";

/**
 * ArticleAction.
 */
function ArticleAction(context, containerDom) {
  this.__init__(context, containerDom);
}

ArticleAction.prototype = {
  __init__: function(context, containerDom) {
    this.__proto__.__proto__.__init__.call(this);
    this._context = context;
    this._containerDom = containerDom;
    this._articleDom = $('<div class="article"></div>');
    this._articleGuid = null;
    this._lastFetchId = null;

    $(this._articleDom).hide();
    $(this._containerDom).append(this._articleDom);
  },

  _handleAjaxSuccess: function(fetchId, jsonResponse) {
    if (this._lastFetchId != fetchId)
      return;

    // Drop the response if it wasn't for the most recent request.
    $(this._articleDom).html(jsonResponse.article);
  },

  _handleAjaxFailure: function(fetchId) {
    if (this._lastFetchId != fetchId)
      return;
  },

  _handleMouseUp: function(event) {
    var self = event.data.self;
  },

  _update: function() {
    if ($(this._articleDom).is(':visible')) {
      // TODO: Add the 'Loading ...' page.
      $(this._articleDom).html('');

      var self = this;
      this._lastFetchId = Math.random();

      $.ajax({
        url: 'article?id=' + this._context.id + '&guid=' + this._articleGuid,
        success: function(r, s, xhr) {
          self._handleAjaxSuccess.call(self, self._lastFetchId, r, s, xhr);
        },
        error: function(xhr, s, e) {
          self._handleAjaxFailure.call(self, self._lastFetchId, xhr, s, e);
        },
        cache: true
      });
    }
  },

  setArticleGuid: function(guid) {
    this.trigger('articleChange');
    this._articleGuid = guid;

    this._update();
  },

  getArticleGuid: function() {
    return this._articleGuid;
  },

  show: function() {
    if (!$(this._articleDom).is(':visible')) {
      this.trigger('show');
      $(this._articleDom).bind('mouseup', {self:this}, this._handleMouseUp);
      $(this._articleDom).show();

      this._update();
    }
  },

  hide: function() {
    if ($(this._articleDom).is(':visible')) {
      $(this._articleDom).unbind('mouseup', this._handleMouseUp);
      this.trigger('hide');
      $(this._articleDom).hide();
    }
  }
}
MergePrototype(ArticleAction.prototype, SignalUserPrototype);

/**
 * SearchAction.
 */
function SearchAction(context, containerDom) {
  this.__init__(context, containerDom);
}

SearchAction.prototype = {
  __init__: function(context, containerDom) {
    this.__proto__.__proto__.__init__.call(this);

    this._context = context;
    this._containerDom = containerDom;
    this._lastQuery = '';
    this._lastGoodQuery = '';
    this._lastSearchId = null;
    this._scrollOffsetX = 0;
    this._resultsDom = $('<div class="results"></div>');

    $(this._resultsDom).hide();
    $(containerDom).append(this._resultsDom);
  },

  /**
   * Redirects browser to article if user clicks on the search results link.
   */
  _handleContainerClick: function(event) {
    if ($(event.target).hasClass('result')) {
      event.preventDefault();

      var self = event.data.self;
      self.trigger('openArticle', [$(event.target).attr('href')]);
    }
  },

  /**
   * Event handler that renders a search results page on successful search.
   */
  _handleSuccess: function(searchId, query, jsonResponse) {
    // Render results iff the current search is the most recent one.
    if (searchId != this._lastSearchId)
      return;

    this._lastGoodQuery = query;

    if ('error' in jsonResponse) {
      // TODO: Display an error to user.
      console.warning(jsonResponse['error']);
      return;
    }

    var html = null;
    var limit = jsonResponse[this._context.name].limit;
    var results = jsonResponse[this._context.name].results;

    if (results.length > 0) {
      // Retrieved result set is not empty- render it.
      var stringList = new Array();

      for (var i = 0; i < results.length; i++) {
        stringList.push('<a class="result" href="');
        stringList.push(results[i][0]);
        stringList.push('">');
        stringList.push(results[i][1]);
        stringList.push('</a>');
      }

      html = stringList.join('');
    } else {
      // Got an empty result set- render the 'No Results' page.
      html = ('No results for "' + query + '".');
    }

    $(this._resultsDom).html(html);
    $(this._contentDom).scrollTop(this._scrollOffsetX);
  },

  /**
   * Event handler that renders a search results page in the case of a search
   * failure.
   */
  _handleFailure: function(searchId, query, xhr, s, e) {
    if (searchId != this._lastSearchId)
      return;

    console.log(xhr.responseText);

    $(this._resultsDom).html('An error occurred while searching for ' + query);
  },

  _update: function() {
    // Do not update anything if the results list element is not visible.
    if (!$(this._resultsDom).is(':visible'))
      return;

    if (this._lastQuery != this._lastGoodQuery) {
      // We need to do a search using new query.
      if (this._lastQuery.length > 0) {
        var self = this;
        this._lastSearchId = Math.random();

        $.ajax({
          url: 'search?id=' + this._context.id + '&q=' +
            encodeURIComponent(this._lastQuery),
          success: function(r, s, xhr) {
            self._handleSuccess(self._lastSearchId, self._lastQuery, r, s, xhr);
          },
          error: function(xhr, s, e) {
            self._handleFailure(self._lastSearchId, self._lastQuery, xhr, s, e);
          },
          cache: true
        });

        // TODO: Display throbbler while waiting for search results.
      } else {
        // Render the 'Empty query' page.
        $(this._resultsDom).html('');
      }
    } else {
      // Search results are up-to-date. Ensure that the scrolling offset is
      // right.
      $(this._containerDom).scrollTop(this._scrollOffsetX);
    }
  },

  show: function() {
    if (!$(this._resultsDom).is(':visible')) {
      this.trigger('show');

      $(this._containerDom).bind('click',
                                 {self:this},
                                 this._handleContainerClick);
      $(this._resultsDom).show();

      this._update();
    }
  },

  hide: function() {
    if ($(this._resultsDom).is(':visible')) {
      this.trigger('hide');

      // Save the scroll position so we can restore it if shown again.
      this._scrollOffsetX = $(this._containerDom).scrollTop();

      $(this._resultsDom).hide();
      $(this._containerDom).unbind('click', this._handleContainerClick);
    }
  },

  setSearchQuery: function(q) {
    if (q != this._lastQuery) {
      this._lastQuery = q;
      this._scrollOffsetX = 0;

      this.trigger('queryChange');
    }

    this._update();
  },

  getSearchQuery: function() {
    return this._lastQuery;
  },

  getScrollOffset: function() {
    if ($(this._resultsDom).is(':visible')) {
      return $(this._containerDom).scrollTop();
    } else {
      return this._scrollOffsetX;
    }
  },

  setScrollOffset: function(offt) {
    this._scrollOffsetX = offt;

    if ($(this._resultsDom).is(':visible'))
      $(this._containerDom).scrollTop(offt);
  }
}; MergePrototype(SearchAction.prototype, SignalUserPrototype);

(function($) {
  var tabsContainer = null;
  var textContainer = null;
  var tabsDom = null;
  var simplifyContext = null;
  var sectionContexts = new Array();
  var selectedSectionIndex = null;
  $.sections = {};

  $.ajax({
    async: false,
    url: 'context',
    success: function (jsonResponse) { simplifyContext = jsonResponse; },
    error: function() { simplifyContext = null; },
    cache: true
  });

  $.sections.init = function(sectionsContainer, contentContainer) {
    var dictionaryContexts = simplifyContext['dicts'];
    var html = new Array();

    tabsContainer = sectionsContainer;
    textContainer = contentContainer;

    // Add the 'All' section.
    // TODO: Implement the 'All' section.
    //html.push('<li><a href="">All</a></li>');

    // Add sections for each dictionary.
    for (var i = 0; i < dictionaryContexts.length; i++) {
      html.push('<li><a href="">');
      html.push(dictionaryContexts[i].name);
      html.push('</a></li>')
    }

    tabsDom = $(html.join(''));

    // Bind custom onclick handers on each section link.
    $('a', tabsDom).each(function(i) {
      var actions = {
        article: new ArticleAction(dictionaryContexts[i], contentContainer),
        search : new SearchAction(dictionaryContexts[i], contentContainer)
      };
      var sectionContext = {
        radioElement     : this,
        actions          : actions,
        activeAction     : actions.search,
        dictionaryContext: dictionaryContexts[i]
      };

      $(this).bind('click', function(event) {
        sectionContext.activeAction.show();
        event.preventDefault();
      });

      actions.article.bind('show', function() {
        handleShowActionEvent.call(this, i, sectionContext);
      });
      actions.search.bind('show', function() {
        handleShowActionEvent.call(this, i, sectionContext);
      });
      actions.article.bind('hide', function() {
        handleHideActionEvent.call(this, i, sectionContext);
      });
      actions.search.bind('hide', function() {
        handleHideActionEvent.call(this, i, sectionContext);
      });
      actions.article.bind('articleChange', function() {
        if (selectedSectionIndex != i)
          sectionContext.activeAction = sectionContext.actions.article;
      });
      actions.search.bind('queryChange', function() {
        if (selectedSectionIndex != i)
          sectionContext.activeAction = sectionContext.actions.search;
      });

      sectionContexts.push(sectionContext);
    });

    $(sectionsContainer).append(tabsDom);
  }

  $.sections.each = function(callback) {
    for (var i = 0; i < sectionContexts.length; i++)
      callback(sectionContexts[i], i);
  }

  $.sections.getSelectionDictionaryId = function() {
    return sectionContexts[selectedSectionIndex].dictionaryContext.id;
  }

  $.sections.getSelectionIndex = function() {
    return selectedSectionIndex;
  }

  $.sections.getSelectionActions = function() {
    return sectionContexts[selectedSectionIndex].actions;
  }

  $.sections.selectSectionByIndex = function(index) {
    sectionContexts[index].activeAction.show();
  }

  $.sections.getSectionCount = function() {
    return $('a', tabsDom).length;
  }

  function handleShowActionEvent(sectionIndex, sectionContext) {
    if (selectedSectionIndex != null)
      sectionContexts[selectedSectionIndex].activeAction.hide();

    sectionContext.activeAction = this;
    selectedSectionIndex = sectionIndex;

    document.title = sectionContext.dictionaryContext.name;

    $('a', tabsDom).removeClass('hl');
    $(sectionContext.radioElement).addClass('hl');
  }

  function handleHideActionEvent(sectionIndex, sectionContext) {
    selectedSectionIndex = null;
  }
})(jQuery);

function dispatch(event) {
  var params = $.deparam.fragment(event.fragment);
  var dictId = parseInt(params['id']);

  switch (params['do']) {
    case 'article':
      var guid = params['guid'];
      var luck = params['luck'];

      with ($('#sexpr')) {
        val(luck);
        focus();
        select();
      }

      $.sections.each(function(context) {
        if (context.dictionaryContext.id != dictId) {
          // Show a lucky article.
        } else {
          // Show article with given GUID.
          context.actions.article.setArticleGuid(guid);
          context.actions.article.show();
        }
      });
      break;
    case 'search':
      var query = params['q'];
      var selectionIndex = $.sections.getSelectionIndex();

      with ($('#sexpr')) {
        val(query);
        focus();
        select();
      }

      $.sections.each(function(context, i) {
        if (i == selectionIndex &&
            context.activeAction != context.actions.search) {
          context.activeAction.hide();
        }

        context.actions.search.setSearchQuery(query);

        if (context.dictionaryContext.id == dictId) {
          var scrollOffset = (params['so']) ? params['so'] : 0;
          context.actions.search.setScrollOffset(scrollOffset);
          context.actions.search.show();
        }
      });
      break;
  }
}

function handleTextChangedEvent(event, text) {
  var selectionIndex = $.sections.getSelectionIndex();

  $.sections.each(function(context, sectionIndex) {
    context.actions.search.setSearchQuery(text);

    // Ensure that search results is shown for currently selected section.
    if (sectionIndex == selectionIndex)
      context.actions.search.show();
  });
}

function handleOpenArticleEvent(dictionaryContext, guid) {
  // Open user-clicked article.
  // Before actually opening the article we need to tell the browser to
  // remember current search in history. To do this, we save the search
  // state in the hash and push it to the browser. We unbind the
  // hashchange handler to avoid any actions from the dispatcher() function.
  $(window).unbind('hashchange', dispatch);

  var searchQuery = this.getSearchQuery();
  var scrollOffset = this.getScrollOffset();

  var intermediate = {
    'do': 'search',
    'id': dictionaryContext.id,
    'q' : searchQuery,
    'so': scrollOffset,
  };

  $.bbq.pushState(intermediate, 2);

  // After we have pushed search state into browser history we need to
  // display the actual article. For this, we rebind previously unbound
  // hashchange handler and then push a new state that the hashchange
  // handler then will catch and will act accordingly.
  setTimeout(function() {
    $(window).bind('hashchange', dispatch);
    $.bbq.pushState({
      'do'  : 'article',
      'id'  : dictionaryContext.id,
      'luck': searchQuery,
      'guid': guid
    }, 2);
  }, 0);
}

$(document).ready(function() {
  $('#sexpr').doTextChangedFeedback();
  $('#sexpr').bind('textchanged', handleTextChangedEvent);

  $('#content').makeContainer();

  $.sections.init($('#navli'), $('#content'));
  $.sections.selectSectionByIndex(0);
  $.sections.getSelectionActions().search.show();

  $.sections.each(function(context) {
    context.actions.search.bind('openArticle', function(guid) {
      handleOpenArticleEvent.call(this, context.dictionaryContext, guid);
    });
  });
});
