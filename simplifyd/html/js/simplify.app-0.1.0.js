"use strict";

/**
 * ArticleAction.
 */
function ArticleAction(context, widget) {
  EventSource.prototype.__init__.call(this);

  this._context = context;
  this._articleAgent = new ArticleAgent();
  this._lastGuid = null;
  this._lastGoodGuid = null;
  this._widget = widget;
}

ArticleAction.prototype = MergePrototype({
  update: function() {
    var self = this;
    var guid = this._lastGuid;

    if (!this._widget.visible()) {
      return;
    } else if (guid == this._lastGoodGuid) {
      this.trigger('updated');
      return;
    }

    this._articleAgent.fetchArticle(this._context, guid,
      function(text) {
        self._lastGoodGuid = guid;
        self._widget.renderArticle(text);
        self.trigger('updated');
      },
      function(details) {
        // TODO: Show something to user.
        console.error('ArticleAction.update: GUID=' + guid);
      }
    );

    this._widget.renderLoadingPage();
  },

  articleGuid: function(guid) {
    if (arguments.length > 0) {
      this._lastGuid = guid;
      this.update();
    } else {
      return this._lastGuid;
    }
  },
}, EventSource.prototype);

/**
 * SearchAction.
 */
function SearchAction(context, widget) {
  EventSource.prototype.__init__.call(this);

  this._context = context;
  this._searchAgent = new SearchAgent();
  this._widget = widget;
  this._lastQuery = '';
  this._lastGoodQuery = '';
}

SearchAction.prototype = MergePrototype({
  update: function() {
    var self = this;
    var query = this._lastQuery;

    // Do nothing if the Search Results widget is not visible or we already
    // have an up-to-date results available.
    if (!this._widget.visible()) {
      return;
    } else if (query == this._lastGoodQuery) {
      this.trigger('updated');
      return;
    }

    if (this._lastQuery.length > 0) {
      this._widget.renderLoadingPage();

      this._searchAgent.search(this._context, query,
        function(results) {
          self._lastGoodQuery = query;
          self._widget.renderResultsPage(results);
          self.trigger('updated');
        },
        function(details) {
          // TODO: Show something to user.
          console.error('SearchAction.update: Query="' + query + '"');
        }
      );
    } else {
      this._widget.renderNoQueryPage();
    }
  },

  searchQuery: function(q) {
    if (arguments.length > 0) {
      this._lastQuery = q;
      this.update();
    } else {
      return this._lastQuery;
    }
  }
}, EventSource.prototype);

(function($) {
  var globalContext;
  var tabContexts = [];
  var tabWidget;
  var overlayWidget;
  $.simplify = {};

  function searchHelper(ctx, q, scrollOffset) {
    ctx.articleWidget.hide();
    ctx.searchWidget.show();
    ctx.savedScrollOffset = scrollOffset;
    ctx.searchAction.searchQuery(q);
  }

  function fetchArticleHelper(ctx, guid, scrollOffset) {
    ctx.searchWidget.hide();
    ctx.articleWidget.show();
    ctx.savedScrollOffset = scrollOffset;
    ctx.articleAction.articleGuid(guid);
  }

  function handleTwOnBeforeClickEvent(event, index) {
    var ctx = tabContexts[this.getIndex()];

    // There will be no context if there was no tab selected before, so
    // be careful.
    if (ctx) {
      ctx.savedScrollOffset = $('#content').scrollTop();
    }
  }

  function handleTwOnClickEvent(event, index) {
    var ctx = tabContexts[index];

    ctx.searchAction.update();
    ctx.articleAction.update();
    document.title = ctx.dictContext.name;
  }

  function handleHashChangeEvent(event) {
    $.simplify.dispatch($.deparam.fragment(event.fragment));
  }

  function handleInlineLookupEvent(text) {
    overlayWidget.show(tabContexts[tabWidget.getIndex()].dictContext, text);
  }

  function handleOpenArticleEvent(guid) {
    // Open currently selected article in the search results widget.

    // Before actually opening the article we need to tell the browser to
    // remember current search in history. To do this, we save the search
    // state in the hash and push it to the browser. We unbind the hashchange
    // handler to avoid any actions from dispatch function.
    $(window).unbind('hashchange', handleHashChangeEvent);

    var tabIndex = tabWidget.getIndex();
    var scrollOffset = $('#content').scrollTop();
    var searchQuery =
      tabContexts[tabWidget.getIndex()].searchAction.searchQuery();

    var intermediate = {
      'do': 'search',
      'ti': tabIndex,
      'q' : searchQuery,
      'so': scrollOffset,
    };

    $.bbq.pushState(intermediate, 2);

    // After we have pushed search state into browser history we need to
    // display the actual article. For this, we rebind previously unbound
    // hashchange handler and then push a new state that the hashchange
    // handler then will catch and will act accordingly.
    setTimeout(function() {
      $(window).bind('hashchange', handleHashChangeEvent);
      $.bbq.pushState({
        'do'  : 'article',
        'ti'  : tabIndex,
        'q'   : searchQuery,
        'guid': guid
      }, 2);
    }, 0);
  }

  function handleReplaceArticleEvent(guid) {
    var tabIndex = tabWidget.getIndex();
    var searchQuery =
      tabContexts[tabWidget.getIndex()].searchAction.searchQuery();

    $.bbq.pushState({
      'do'  : 'article',
      'ti'  : tabIndex,
      'q'   : searchQuery,
      'guid': guid
    }, 2);
  }

  function createOverlayWidget() {
    overlayWidget = new OverlayArticleWidget($('body'), {
      constraintElement: $('#content')[0]
    });
  }

  function initializeTabWidgetPanes() {
    $('#content > div').each(function(index) {
      var srwidget = new SearchResultsWidget(this, {
        containerClass: 'search-result-container'
      });
      var awidget = new ArticleWidget(this, {
        containerClass: 'article-container'
      });
      var tabContext = {
        dictContext: globalContext.dicts[index],
        searchAction: new SearchAction(globalContext.dicts[index], srwidget),
        articleAction: new ArticleAction(globalContext.dicts[index], awidget),
        searchWidget: srwidget,
        articleWidget: awidget,
        savedScrollOffset: 0
      };

      awidget.bind('inlineLookup', handleInlineLookupEvent);
      awidget.bind('replaceArticle', handleReplaceArticleEvent);
      srwidget.bind('openArticle', handleOpenArticleEvent);

      var restoreScrollFun = function() {
        $('#content').scrollTop(tabContext.savedScrollOffset);
      };
      tabContext.searchAction.bind('updated', restoreScrollFun);
      tabContext.articleAction.bind('updated', restoreScrollFun);

      tabContexts.push(tabContext);
    });
  }

  function createTabWidget() {
    var tabsHtml = [];
    var localContexts = globalContext.dicts;

    // Populate tabs pane.
    for (var i = 0; i < localContexts.length; i++) {
      tabsHtml.push('<a href="">');
      tabsHtml.push(localContexts[i].name);
      tabsHtml.push('</a>');
    }

    $('#nav').html(tabsHtml.join(''));

    // Create an empty pane in the content container for each tab.
    $('#content').html((new Array(localContexts.length+1).join('<div></div>')));

    // Finally, create the tab widget.
    $('#nav').tabs('#content > div', {
      initialIndex : null,
      onBeforeClick: handleTwOnBeforeClickEvent,
      onClick      : handleTwOnClickEvent
    });

    initializeTabWidgetPanes();
  }

  $.ajax({
    async  : false,
    url    : 'context',
    success: function(ctx) { globalContext = ctx; },
    error  : function() { globalContext = null; },
    cache  : true
  });

  $.simplify.initialize = function() {
    createTabWidget();
    createOverlayWidget();

    $(window).bind('hashchange', handleHashChangeEvent);

    tabWidget = $('#nav').data('tabs');
    tabWidget.click(0);
  };

  $.simplify.dispatch = function(params) {
    var tabIndex = parseInt(params.ti);
    var query = params.q;

    with ($('#query-edit')) {
      setText(query || '', true);
      focus();
      select();
    }

    switch (params.do) {
      case 'article':
        // TODO: Load a lucky result for other dictionaries.
        fetchArticleHelper(tabContexts[tabIndex], params.guid, 0);
        tabWidget.click(tabIndex);
        break;
      case 'search':
        searchHelper(tabContexts[tabIndex], query, params.so || 0);
        tabWidget.click(tabIndex);

        for (var i = 0; i < tabContexts.length; i++) {
          if (i != tabIndex) {
            searchHelper(tabContexts[i], query, 0);
          }
        }
        break;
    }
  };

  $.simplify.globalSearch = function(query) {
    for (var i = 0; i < tabContexts.length; i++) {
      searchHelper(tabContexts[i], query, 0);
    }
  };
})(jQuery);

function handleTextChangedEvent(event, text) {
  $.simplify.globalSearch(text);
}

$(document).ready(function() {
  $('#query-edit').bind('textchanged', handleTextChangedEvent);
  $('#query-edit').startInstantFeedback();

  $.simplify.initialize();
  $.simplify.dispatch($.deparam.fragment());
});
