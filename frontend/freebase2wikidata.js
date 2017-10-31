/**
 * freebase2wikidata.js
 *
 * Wikidata user script for the migration of Freebase facts into Wikidata.
 *
 * @author: Thomas Steiner (tomac@google.com)
 * @author: Thomas Pellissier Tanon (thomas@pellissier-tanon.fr)
 * @license: CC0 1.0 Universal (CC0 1.0)
 */

/* jshint
  bitwise:true,
  curly: true,
  indent: 2,
  immed: true,
  latedef: nofunc,
  loopfunc: true,
  maxlen: 80,
  newcap: true,
  browser: true,
  noempty: true,
  nonbsp: true,
  nonew: true,
  quotmark: single,
  shadow: true,
  strict: true,
  trailing: true,
  undef: true,
  unused: true
*/

/* global
  console, mw, $, module, HTML_TEMPLATES, OO, wikibase
*/

$(function() {

  var async = module.exports;

  var DEBUG = JSON.parse(localStorage.getItem('f2w_debug')) || false;
  var FAKE_OR_RANDOM_DATA =
      JSON.parse(localStorage.getItem('f2w_fakeOrRandomData')) || false;

  var CACHE_EXPIRY = 60 * 60 * 1000;

  var WIKIDATA_ENTITY_DATA_URL =
      'https://www.wikidata.org/wiki/Special:EntityData/{{qid}}.json';
  var FREEBASE_ENTITY_DATA_URL =
      'https://tools.wmflabs.org/wikidata-primary-sources/entities/{{qid}}';
  var FREEBASE_STATEMENT_APPROVAL_URL =
      'https://tools.wmflabs.org/wikidata-primary-sources/statements/{{id}}' +
      '?state={{state}}&user={{user}}';
  var FREEBASE_STATEMENT_SEARCH_URL =
    'https://tools.wmflabs.org/wikidata-primary-sources/statements/all';
  var FREEBASE_DATASETS =
    'https://tools.wmflabs.org/wikidata-primary-sources/datasets/all';
  var FREEBASE_SOURCE_URL_BLACKLIST = 'https://www.wikidata.org/w/api.php' +
      '?action=parse&format=json&prop=text' +
      '&page=Wikidata:Primary_sources_tool/URL_blacklist';
  var FREEBASE_SOURCE_URL_WHITELIST = 'https://www.wikidata.org/w/api.php' +
      '?action=parse&format=json&prop=text' +
      '&page=Wikidata:Primary_sources_tool/URL_whitelist';

  var WIKIDATA_API_COMMENT =
      'Added via [[Wikidata:Primary sources tool]]';

  var STATEMENT_STATES = {
    new: 'new',
    approved: 'approved',
    rejected: 'rejected',
    duplicate: 'duplicate',
    blacklisted: 'blacklisted'
  };
  var STATEMENT_FORMAT = 'QuickStatement';

  /* jshint ignore:start */
  /* jscs: disable */
  var HTML_TEMPLATES = {
    qualifierHtml:
        '<div class="wikibase-listview">' +
          '<div class="wikibase-snaklistview listview-item">' +
            '<div class="wikibase-snaklistview-listview">' +
              '<div class="wikibase-snakview listview-item">' +
                '<div class="wikibase-snakview-property-container">' +
                  '<div class="wikibase-snakview-property" dir="auto">' +
                    '{{qualifier-property-html}}' +
                  '</div>' +
                '</div>' +
                '<div class="wikibase-snakview-value-container" dir="auto">' +
                  '<div class="wikibase-snakview-typeselector"></div>' +
                  '<div class="wikibase-snakview-value wikibase-snakview-variation-valuesnak" style="height: auto; width: 100%;">' +
                    '<div class="valueview valueview-instaticmode" aria-disabled="false">' +
                      '{{qualifier-object}}' +
                    '</div>' +
                  '</div>' +
                '</div>' +
              '</div>' +
              '<!-- wikibase-listview -->' +
            '</div>' +
          '</div>' +
        '</div>',
    sourceHtml:
        '<div class="wikibase-referenceview listview-item wikibase-toolbar-item new-source">' + // Remove wikibase-reference-d6e3ab4045fb3f3feea77895bc6b27e663fc878a wikibase-referenceview-d6e3ab4045fb3f3feea77895bc6b27e663fc878a
          '<div class="wikibase-referenceview-heading new-source">' +
            '<div class="wikibase-edittoolbar-container wikibase-toolbar-container">' +
              '<span class="wikibase-toolbar wikibase-toolbar-item wikibase-toolbar-container">' +
                '<span class="wikibase-toolbarbutton wikibase-toolbar-item wikibase-toolbar-button wikibase-toolbar-button-add">' +
                  '<a class="f2w-button f2w-source f2w-approve" href="#" data-statement-id="{{statement-id}}" data-property="{{data-property}}" data-object="{{data-object}}" data-source="{{data-source}}" data-qualifiers="{{data-qualifiers}}"><span class="wb-icon"></span>approve reference</a>' +
                '</span>' +
              '</span>' +
              ' ' +
              /* TODO: Broken by the last changes.
              '<span class="wikibase-toolbar wikibase-toolbar-item wikibase-toolbar-container">' +
                '[' +
                '<span class="wikibase-toolbarbutton wikibase-toolbar-item wikibase-toolbar-button wikibase-toolbar-button-edit">' +
                  '<a class="f2w-button f2w-source f2w-edit" href="#" data-statement-id="{{statement-id}}" data-property="{{data-property}}" data-object="{{data-object}}" data-source-property="{{data-source-property}}" data-source-object="{{data-source-object}}" data-qualifiers="{{data-qualifiers}}">edit</a>' +
                '</span>' +
                ']' +
              '</span>' +*/
              '<span class="wikibase-toolbar wikibase-toolbar-item wikibase-toolbar-container">' +
                '<span class="wikibase-toolbarbutton wikibase-toolbar-item wikibase-toolbar-button wikibase-toolbar-button-remove">' +
                  '<a class="f2w-button f2w-source f2w-reject" href="#" data-statement-id="{{statement-id}}" data-property="{{data-property}}" data-object="{{data-object}}" data-source="{{data-source}}" data-qualifiers="{{data-qualifiers}}"><span class="wb-icon"></span>reject reference</a>' +
                '</span>' +
              '</span>' +
            '</div>' +
          '</div>' +
          '<div class="wikibase-referenceview-listview">' +
            '<div class="wikibase-snaklistview listview-item">' +
              '<div class="wikibase-snaklistview-listview">' +
                '{{source-html}}' +
                '<!-- wikibase-listview -->' +
              '</div>' +
            '</div>' +
            '<!-- [0,*] wikibase-snaklistview -->' +
          '</div>' +
        '</div>',
    sourceItemHtml:
        '<div class="wikibase-snakview listview-item">' +
          '<div class="wikibase-snakview-property-container">' +
            '<div class="wikibase-snakview-property" dir="auto">' +
              '{{source-property-html}}' +
            '</div>' +
          '</div>' +
          '<div class="wikibase-snakview-value-container" dir="auto">' +
            '<div class="wikibase-snakview-typeselector"></div>' +
            '<div class="wikibase-snakview-value wikibase-snakview-variation-valuesnak" style="height: auto;">' +
              '<div class="valueview valueview-instaticmode" aria-disabled="false">' +
                '{{source-object}}' +
              '</div>' +
            '</div>' +
          '</div>' +
        '</div>',
    statementViewHtml:
        '<div class="wikibase-statementview listview-item wikibase-toolbar-item new-object">' + // Removed class wikibase-statement-q31$8F3B300A-621A-4882-B4EE-65CE7C21E692
          '<div class="wikibase-statementview-rankselector">' +
            '<div class="wikibase-rankselector ui-state-disabled">' +
              '<span class="ui-icon ui-icon-rankselector wikibase-rankselector-normal" title="Normal rank"></span>' +
            '</div>' +
          '</div>' +
          '<div class="wikibase-statementview-mainsnak-container">' +
            '<div class="wikibase-statementview-mainsnak" dir="auto">' +
              '<!-- wikibase-snakview -->' +
              '<div class="wikibase-snakview">' +
                '<div class="wikibase-snakview-property-container">' +
                  '<div class="wikibase-snakview-property" dir="auto">' +
                    '{{property-html}}' +
                  '</div>' +
                '</div>' +
                '<div class="wikibase-snakview-value-container" dir="auto">' +
                  '<div class="wikibase-snakview-typeselector"></div>' +
                  '<div class="wikibase-snakview-value wikibase-snakview-variation-valuesnak" style="height: auto;">' +
                    '<div class="valueview valueview-instaticmode" aria-disabled="false">{{object}}</div>' +
                  '</div>' +
                '</div>' +
              '</div>' +
            '</div>' +
            '<div class="wikibase-statementview-qualifiers">' +
              '{{qualifiers}}' +
              '<!-- wikibase-listview -->' +
            '</div>' +
          '</div>' +
          '<!-- wikibase-toolbar -->' +
          '<span class="wikibase-toolbar-container wikibase-edittoolbar-container">' +
            '<span class="wikibase-toolbar-item wikibase-toolbar wikibase-toolbar-container">' +
              '<span class="wikibase-toolbarbutton wikibase-toolbar-item wikibase-toolbar-button wikibase-toolbar-button-add">' +
                '<a class="f2w-button f2w-property f2w-approve" href="#" data-statement-id="{{statement-id}}" data-property="{{data-property}}" data-object="{{data-object}}" data-qualifiers="{{data-qualifiers}}" data-sources="{{data-sources}}"><span class="wb-icon"></span>approve claim</a>' +
              '</span>' +
            '</span>' +
            ' ' +
            '<span class="wikibase-toolbar-item wikibase-toolbar wikibase-toolbar-container">' +
              '<span class="wikibase-toolbarbutton wikibase-toolbar-item wikibase-toolbar-button wikibase-toolbar-button-remove">' +
                '<a class="f2w-button f2w-property f2w-reject" href="#" data-statement-id="{{statement-id}}" data-property="{{data-property}}" data-object="{{data-object}}" data-qualifiers="{{data-qualifiers}}" data-sources="{{data-sources}}"><span class="wb-icon"></span>reject claim</a>' +
              '</span>' +
            '</span>' +
          '</span>' +
          '<div class="wikibase-statementview-references-container">' +
            '<div class="wikibase-statementview-references-heading">' +
              '<a class="ui-toggler ui-toggler-toggle ui-state-default">' + // Removed ui-toggler-toggle-collapsed
                '<span class="ui-toggler-icon ui-icon ui-icon-triangle-1-s ui-toggler-icon3dtrans"></span>' +
                '<span class="ui-toggler-label">{{references}}</span>' +
              '</a>' +
            '</div>' +
            '<div class="wikibase-statementview-references wikibase-toolbar-item">' + // Removed style="display: none;"
              '<!-- wikibase-listview -->' +
              '<div class="wikibase-listview">' +
                '{{sources}}' +
              '</div>' +
              '<div class="wikibase-addtoolbar-container wikibase-toolbar-container">' +
                '<!-- [' +
                '<span class="wikibase-toolbarbutton wikibase-toolbar-item wikibase-toolbar-button wikibase-toolbar-button-add">' +
                  '<a href="#">add reference</a>' +
                '</span>' +
                '] -->' +
              '</div>' +
            '</div>' +
          '</div>' +
        '</div>',
    mainHtml:
        '<div class="wikibase-statementgroupview listview-item" id="{{property}}">' +
          '<div class="wikibase-statementgroupview-property new-property">' +
            '<div class="wikibase-statementgroupview-property-label" dir="auto">' +
              '{{property-html}}' +
            '</div>' +
          '</div>' +
          '<!-- wikibase-statementlistview -->' +
          '<div class="wikibase-statementlistview wikibase-toolbar-item">' +
            '<div class="wikibase-statementlistview-listview">' +
              '<!-- [0,*] wikibase-statementview -->' +
              '{{statement-views}}' +
            '</div>' +
            '<!-- [0,1] wikibase-toolbar -->' +
            '<span class="wikibase-toolbar-container"></span>' +
            '<span class="wikibase-toolbar-wrapper">' +
              '<div class="wikibase-addtoolbar-container wikibase-toolbar-container">' +
                '<!-- [' +
                '<span class="wikibase-toolbarbutton wikibase-toolbar-item wikibase-toolbar-button wikibase-toolbar-button-add">' +
                  '<a href="#">add</a>' +
                '</span>' +
                '] -->' +
              '</div>' +
            '</span>' +
          '</div>' +
        '</div>'
  };
  /* jscs: enable */
  /* jshint ignore:end */

  var debug = {
    log: function(message) {
      if (DEBUG) {
        console.log('F2W: ' + message);
      }
    }
  };

  var qid = null;
  function getQid() {
    var qidRegEx = /^Q\d+$/;
    var title = mw.config.get('wgTitle');
    return qidRegEx.test(title) ? title : false;
  }

  (function generateNav() {
    $('#mw-panel').append('<div class="portal" role="navigation" id="p-ps-navigation" aria-labelledby="p-ps-navigation-label"><h3 id="p-ps-navigation-label">Browse Primary Sources</h3></div>');
    var navigation =  $('#p-ps-navigation');
    navigation.append('<div class="body"><ul id="p-ps-nav-list"></ul></div>');
    $('#p-ps-nav-list').before('<a href="#" id="n-ps-anchor-btt" title="move to top">&#x25B2 back to top &#x25B2</a>');
    $('#n-ps-anchor-btt').click(function(e) {
      e.preventDefault();
      $('html,body').animate({
        scrollTop: 0
      }, 0);
    });
    scrollFollowTop(navigation);
  })();

  function scrollFollowTop($sidebar) {
    var $window = $(window),
        offset = $sidebar.offset(),
        topPadding = 15;

    $window.scroll(function() {
      if ($window.scrollTop() > offset.top) {
        $sidebar.stop().animate({
          marginTop: $window.scrollTop() - offset.top + topPadding
        }, 200);
        } else {
        $sidebar.stop().animate({
          marginTop: 0
        }, 200);
      }
    });
  }

  var anchorList = [];

  function alphaPos(text){
    if(text <= anchorList[0]){
      return 0;
    }
    for(var i = 0; i < anchorList.length -1; i++){
      if(text > anchorList[i] && text < anchorList[i+1]){
        return i+1;
      }
    }
    return anchorList.length;
  }

  function appendToNav(container){
    var firstNewObj = $(container).find('.new-object')[0];
    if (firstNewObj) {
      var anchor = {
        title : $(container).find('.wikibase-statementgroupview-property-label'),
        target : $(firstNewObj).find('.valueview-instaticmode')[0]
      };
      var text_nospace = anchor.title.text().replace(/\W/g, '');
      var text_space = anchor.title.text().replace(/[^\w\s]/g, '');
      if(anchorList.indexOf(text_nospace) == -1){
        var pos = alphaPos(text_nospace);
        anchorList.splice(pos, 0, text_nospace);
        if(pos === 0){
          $('#p-ps-nav-list').prepend('<li id="n-ps-anchor-' + text_nospace + '"><a href="#" title="move to ' + text_space + '">' + text_space + '</a></li>');
        }
        else{
          $('#n-ps-anchor-' + anchorList[pos-1]).after('<li id="n-ps-anchor-' + text_nospace + '"><a href="#" title="move to ' + text_space + '">' + text_space + '</a></li>');
        }
        $('#n-ps-anchor-' + text_nospace).click(function(e) {
          e.preventDefault();
          anchor.target.scrollIntoView();
        });
      }
    }
  }

  var dataset = mw.cookie.get('ps-dataset', null, '');
  var windowManager;

  (function init() {

    // Add random Primary Sources item button
    (function createRandomFreebaseItemLink() {
      var datasetLabel = (dataset === '') ? 'Primary Sources' : dataset;
      var portletLink = $(mw.util.addPortletLink(
        'p-navigation',
        '#',
        'Random ' + datasetLabel + ' item',
        'n-random-ps',
        'Load a new random ' + datasetLabel + ' item',
        '',
        '#n-help'
      ));
      portletLink.children().click(function(e) {
        e.preventDefault();
        e.target.innerHTML = '<img src="https://upload.wikimedia.org/' +
            'wikipedia/commons/f/f8/Ajax-loader%282%29.gif" class="ajax"/>';
        $.ajax({
          url: FREEBASE_ENTITY_DATA_URL.replace(/\{\{qid\}\}/, 'any') +
              '?dataset=' + dataset
        }).done(function(data) {
          var newQid = data[0].statement.split(/\t/)[0];
          document.location.href = 'https://www.wikidata.org/wiki/' + newQid;
        }).fail(function() {
          return reportError('Could not obtain random Primary Sources item');
        });
      });

      mw.loader.using(
          ['jquery.tipsy', 'oojs-ui', 'wikibase.dataTypeStore'], function() {
        windowManager = new OO.ui.WindowManager();
        $('body').append(windowManager.$element);

        var configButton = $('<span>')
          .attr({
            id: 'ps-config-button',
            title: 'Primary Sources options'
          })
          .tipsy()
          .appendTo(portletLink);
        configDialog(configButton);

        var listButton = $(mw.util.addPortletLink(
            'p-tb',
            '#',
            'Primary Sources list',
            'n-ps-list',
            'List statements from Primary Sources'
          ));
        listDialog(listButton);
      });
    })();

    // Handle clicks on approve/edit/reject buttons
    (function addClickHandlers() {

      var contentDiv = document.getElementById('content');
      contentDiv.addEventListener('click', function(event) {
        var classList = event.target.classList;
        if (!classList.contains('f2w-button')) {
          return;
        }
        event.preventDefault();
        event.target.innerHTML = '<img src="https://upload.wikimedia.org/' +
            'wikipedia/commons/f/f8/Ajax-loader%282%29.gif" class="ajax"/>';
        var statement = event.target.dataset;
        var id = statement.statementId;
        if (classList.contains('f2w-property')) {
          if (classList.contains('f2w-approve')) {
            // Approve property
            var predicate = statement.property;
            var object = statement.object;
            var qualifiers = JSON.parse(statement.qualifiers);
            var sources = JSON.parse(statement.sources);
            createClaim(qid, predicate, object, qualifiers)
              .fail(function(error) {
                return reportError(error);
              }).done(function(data) {
                // Sources, don't validate the statement
                if (sources.length > 0) {
                  return document.location.reload();
                }

                setStatementState(id, STATEMENT_STATES.approved)
                .done(function() {
                  debug.log('Approved property statement ' + id);
                  if (data.pageinfo && data.pageinfo.lastrevid) {
                    document.location.hash = 'revision=' +
                        data.pageinfo.lastrevid;
                  }
                  return document.location.reload();
                });
              });
          } else if (classList.contains('f2w-reject')) {
            // Get the statements ids to reject
            var idsSet = {};
            idsSet[id] = true;
            var sources = JSON.parse(statement.sources);
            sources.forEach(function(source) {
              idsSet[source[0].sourceId] = true;
            });

            // Reject statements
            var rejectPromises = [];
            for (var statementId in idsSet) {
              rejectPromises.push(
                  setStatementState(statementId, STATEMENT_STATES.rejected));
            }
            $.when.apply(this, rejectPromises).done(function() {
              debug.log('Disapproved statement');
              return document.location.reload();
            });
          }
        } else if (classList.contains('f2w-source')) {
          if (classList.contains('f2w-approve')) {
            // Approve source
            var predicate = statement.property;
            var object = statement.object;
            var source = JSON.parse(statement.source);
            var qualifiers = JSON.parse(statement.qualifiers);
            getClaims(qid, predicate, function(err, claims) {
              var objectExists = false;
              for (var i = 0, lenI = claims.length; i < lenI; i++) {
                var claim = claims[i];
                if (
                    claim.mainsnak.snaktype === 'value' &&
                    jsonToTsvValue(claim.mainsnak.datavalue) === object
                ) {
                  objectExists = true;
                  break;
                }
              }
              if (objectExists) {
                createReference(qid, predicate, object, source,
                    function(error, data) {
                  if (error) {
                    return reportError(error);
                  }
                  setStatementState(id, STATEMENT_STATES.approved)
                  .done(function() {
                    debug.log('Approved source statement ' + id);
                    if (data.pageinfo && data.pageinfo.lastrevid) {
                      document.location.hash = 'revision=' +
                          data.pageinfo.lastrevid;
                    }
                    return document.location.reload();
                  });
                });
              } else {
                createClaimWithReference(qid, predicate, object, qualifiers,
                  source)
                  .fail(function(error) {
                    return reportError(error);
                  })
                  .done(function(data) {
                    setStatementState(id, STATEMENT_STATES.approved)
                    .done(function() {
                      debug.log('Approved source statement ' + id);
                      if (data.pageinfo && data.pageinfo.lastrevid) {
                        document.location.hash = 'revision=' +
                            data.pageinfo.lastrevid;
                      }
                      return document.location.reload();
                    });
                  });
              }
            });
          } else if (classList.contains('f2w-reject')) {
            // Reject source
            setStatementState(id, STATEMENT_STATES.rejected).done(function() {
              debug.log('Disapproved source statement ' + id);
              return document.location.reload();
            });
          } else if (classList.contains('f2w-edit')) {
            var a = document.getElementById('f2w-' + id);

            var onClick = function(e) {
              if (isUrl(e.target.textContent)) {
                a.style.textDecoration = 'none';
                a.href = e.target.textContent;
              } else {
                a.style.textDecoration = 'line-through';
              }
            };
            a.addEventListener('input', onClick);

            a.addEventListener('blur', function() {
              a.removeEventListener(onClick);
              a.onClick = function() { return true; };
              a.contentEditable = false;
              event.target.textContent = 'edit';
              var buttons = event.target.parentNode.parentNode
                  .querySelectorAll('a');
              [].forEach.call(buttons, function(button) {
                button.dataset.sourceObject = a.href;
              });
            });

            a.contentEditable = true;
          }
        }
      });
    })();

    if ((mw.config.get('wgPageContentModel') !== 'wikibase-item') ||
        (mw.config.get('wgIsRedirect')) ||
        // Do not run on diff pages
        (document.location.search.indexOf('&diff=') !== -1) ||
        // Do not run on history pages
        (document.location.search.indexOf('&action=history') !== -1)) {
      return;
    }
    qid = getQid();
    if (!qid) {
      return debug.log('Did not manage to load the QID.');
    }

    async.parallel({
      blacklistedSourceUrls: getBlacklistedSourceUrlsWithCallback,
      whitelistedSourceUrls: getWhitelistedSourceUrlsWithCallback,
      wikidataEntityData: getWikidataEntityData.bind(null, qid),
      freebaseEntityData: getFreebaseEntityData.bind(null, qid),
    }, function(err, results) {
      if (err) {
        reportError(err);
      }
      // See https://www.mediawiki.org/wiki/Wikibase/Notes/JSON
      var wikidataEntityData = results.wikidataEntityData;
      var wikidataClaims = wikidataEntityData.claims || {};

      var freebaseEntityData = results.freebaseEntityData;
      var blacklistedSourceUrls = results.blacklistedSourceUrls;
      var freebaseClaims = parseFreebaseClaims(freebaseEntityData,
          blacklistedSourceUrls);

      matchClaims(wikidataClaims, freebaseClaims);
    });
  })();

  function reportError(error) {
    mw.notify(
      error,
      {autoHide: false, tag: 'ps-error'}
    );
  }

  function configDialog(button) {
    function ConfigDialog(config) {
      ConfigDialog.super.call(this, config);
    }
    OO.inheritClass(ConfigDialog, OO.ui.ProcessDialog);
    ConfigDialog.static.name = 'ps-config';
    ConfigDialog.static.title = 'Primary Sources configuration';
    ConfigDialog.static.size = 'large';
    ConfigDialog.static.actions = [
      {action: 'save', label: 'Save', flags: ['primary', 'constructive']},
      {label: 'Cancel', flags: 'safe'}
    ];

    ConfigDialog.prototype.initialize = function() {
      ConfigDialog.super.prototype.initialize.apply(this, arguments);

      this.dataset = new OO.ui.ButtonSelectWidget({
        items: [new OO.ui.ButtonOptionWidget({
          data: '',
          label: 'All sources'
        })]
      });

      var dialog = this;
      getPossibleDatasets(function(datasets) {
        for (var datasetId in datasets) {
          dialog.dataset.addItems([new OO.ui.ButtonOptionWidget({
            data: datasetId,
            label: datasetId,
          })]);
        }
      });

      this.dataset.selectItemByData(dataset);

      var fieldset = new OO.ui.FieldsetLayout({
        label: 'Dataset to use'
      });
      fieldset.addItems([this.dataset]);

      this.panel = new OO.ui.PanelLayout({
        padded: true,
        expanded: false
      });
      this.panel.$element.append(fieldset.$element);
      this.$body.append(this.panel.$element);
    };

    ConfigDialog.prototype.getActionProcess = function(action) {
      if (action === 'save') {
        mw.cookie.set('ps-dataset', this.dataset.getSelectedItem().getData());
        return new OO.ui.Process(function() {
          location.reload();
        });
      }

      return ConfigDialog.super.prototype.getActionProcess.call(this, action);
    };

    ConfigDialog.prototype.getBodyHeight = function() {
      return this.panel.$element.outerHeight(true);
    };

    windowManager.addWindows([new ConfigDialog()]);

    button.click(function() {
      windowManager.openWindow('ps-config');
    });
  }

  function getPossibleDatasets(callback) {
    var now = Date.now();
    if (localStorage.getItem('f2w_dataset')) {
      var blacklist = JSON.parse(localStorage.getItem('f2w_dataset'));
      if (!blacklist.timestamp) {
        blacklist.timestamp = 0;
      }
      if (now - blacklist.timestamp < CACHE_EXPIRY) {
        return callback(blacklist.data);
      }
    }
    $.ajax({
      url: FREEBASE_DATASETS
    }).done(function(data) {
      localStorage.setItem('f2w_dataset', JSON.stringify({
        timestamp: now,
        data: data
      }));
      return callback(data);
    }).fail(function() {
      debug.log('Could not obtain datasets');
    });
  }

  function isBlackListedBuilder(blacklistedSourceUrls) {
    return function(url) {
      try {
        var url = new URL(url);
      } catch (e) {
        return false;
      }

      for (var i in blacklistedSourceUrls) {
        if (url.host.indexOf(blacklistedSourceUrls[i]) !== -1) {
          return true;
        }
      }
      return false;
    };
  }

  function parsePrimarySourcesStatement(statement, isBlacklisted) {
    var id = statement.id;
    var line = statement.statement.split(/\t/);
    var subject = line[0];
    var predicate = line[1];
    var object = line[2];
    var qualifiers = [];
    var source = [];
    var key = object;
    // Handle any qualifiers and/or sources
    var qualifierKeyParts = [];
    var lineLength = line.length;
    for (var i = 3; i < lineLength; i += 2) {
      if (i === lineLength - 1) {
        debug.log('Malformed qualifier/source pieces');
        break;
      }
      if (/^P\d+$/.exec(line[i])) {
        var qualifierKey = line[i] + '\t' + line[i + 1];
        qualifiers.push({
          qualifierProperty: line[i],
          qualifierObject: line[i + 1],
          key: qualifierKey
        });
        qualifierKeyParts.push(qualifierKey);
      } else if (/^S\d+$/.exec(line[i])) {
        source.push({
          sourceProperty: line[i].replace(/^S/, 'P'),
          sourceObject: line[i + 1],
          sourceType: (tsvValueToJson(line[i + 1])).type,
          sourceId: id,
          key: line[i] + '\t' + line[i + 1]
        });
      }

      qualifierKeyParts.sort();
      key += '\t' + qualifierKeyParts.join('\t');

      // Filter out blacklisted source URLs
      source = source.filter(function(source) {
        if (source.sourceType === 'url') {
          var url = source.sourceObject.replace(/^"/, '').replace(/"$/, '');
          var blacklisted = isBlacklisted(url);
          if (blacklisted) {
            debug.log('Encountered blacklisted source url ' + url);
            (function(currentId, currentUrl) {
              setStatementState(currentId, STATEMENT_STATES.blacklisted)
                  .done(function() {
                debug.log('Automatically blacklisted statement ' +
                    currentId + ' with blacklisted source url ' +
                    currentUrl);
              });
            })(id, url);
          }
          // Return the opposite, i.e., the whitelisted URLs
          return !blacklisted;
        }
        return true;
      });
    }

    return {
      id: id,
      subject: subject,
      predicate: predicate,
      object: object,
      qualifiers: qualifiers,
      source: source,
      key: key
    };
  }

  function parseFreebaseClaims(freebaseEntityData, blacklistedSourceUrls) {
    var isBlacklisted = isBlackListedBuilder(blacklistedSourceUrls);

    var freebaseClaims = {};
    /* jshint ignore:start */
    /* jscs: disable */
    if (DEBUG) {
      if (qid === 'Q4115189') {
        // The sandbox item can be written to
        document.getElementById('content').style.backgroundColor = 'lime';
      }
    }
    if (FAKE_OR_RANDOM_DATA) {
      freebaseEntityData.push({
        statement: qid + '\tP31\tQ1\tP580\t+1840-01-01T00:00:00Z/9\tS143\tQ48183',
        state: STATEMENT_STATES.new,
        id: 0,
        format: STATEMENT_FORMAT
      });
      freebaseEntityData.push({
        statement: qid + '\tP108\tQ95\tS854\t"http://research.google.com/pubs/vrandecic.html"',
        state: STATEMENT_STATES.new,
        id: 0,
        format: STATEMENT_FORMAT
      });
      freebaseEntityData.push({
        statement: qid + '\tP108\tQ8288\tP582\t+2013-09-30T00:00:00Z/10\tS854\t"http://simia.net/wiki/Denny"\tS813\t+2015-02-14T00:00:00Z/11',
        state: STATEMENT_STATES.new,
        id: 0,
        format: STATEMENT_FORMAT
      });
      freebaseEntityData.push({
        statement: qid + '\tP1451\ten:"foo bar"\tP582\t+2013-09-30T00:00:00Z/10\tS854\t"http://www.ebay.com/itm/GNC-Mens-Saw-Palmetto-Formula-60-Tablets/301466378726?pt=LH_DefaultDomain_0&hash=item4630cbe1e6"',
        state: STATEMENT_STATES.new,
        id: 0,
        format: STATEMENT_FORMAT
      });
      freebaseEntityData.push({
        statement: qid + '\tP108\tQ8288\tP582\t+2013-09-30T00:00:00Z/10\tS854\t"https://lists.wikimedia.org/pipermail/wikidata-l/2013-July/002518.html"',
        state: STATEMENT_STATES.new,
        id: 0,
        format: STATEMENT_FORMAT
      });
      freebaseEntityData.push({
        statement: qid + '\tP1082\t-1234',
        state: STATEMENT_STATES.new,
        id: 0,
        format: STATEMENT_FORMAT
      });
      freebaseEntityData.push({
        statement: qid + '\tP625\t@-12.12334556/23.1234',
        state: STATEMENT_STATES.new,
        id: 0,
        format: STATEMENT_FORMAT
      });
      freebaseEntityData.push({
        statement: qid + '\tP646\t"/m/05zhl_"',
        state: STATEMENT_STATES.new,
        id: 0,
        format: STATEMENT_FORMAT
      });
      freebaseEntityData.push({
        statement: qid + '\tP569\t+1840-01-01T00:00:00Z/11\tS854\t"https://lists.wikimedia.org/pipermail/wikidata-l/2013-July/002518.html"',
        state: STATEMENT_STATES.new,
        id: 0,
        format: STATEMENT_FORMAT
      });
    }
    /* jscs: enable */
    /* jshint ignore:end */
    // Unify statements, as some statements may appear more than once
    var statementUnique = function(haystack, needle) {
      for (var i = 0, lenI = haystack.length; i < lenI; i++) {
        if (haystack[i].statement === needle) {
          return i;
        }
      }
      return -1;
    };
    var statements = freebaseEntityData.filter(function(freebaseEntity, index, self) {
      return statementUnique(self, freebaseEntity.statement) === index;
    })
    // Only show v1 new statements
    .filter(function(freebaseEntity) {
      return freebaseEntity.format === STATEMENT_FORMAT &&
          freebaseEntity.state === STATEMENT_STATES.new;
    })
    .map(function(freebaseEntity) {
      return parsePrimarySourcesStatement(freebaseEntity, isBlacklisted);
    });

    preloadEntityLabels(statements);

    statements.forEach(function(statement) {
      var predicate = statement.predicate;
      var key = statement.key;

      freebaseClaims[predicate] = freebaseClaims[predicate] || {};
      if (!freebaseClaims[predicate][key]) {
        freebaseClaims[predicate][key] = {
          id: statement.id,
          object: statement.object,
          qualifiers: statement.qualifiers,
          sources: []
        };
      }

      if (statement.source.length > 0) {
        // TODO: find duplicates
        freebaseClaims[predicate][key].sources.push(statement.source);
      }
    });
    return freebaseClaims;
  }

  function computeCoordinatesPrecision(latitude, longitude) {
    return Math.min(
        Math.pow(10, -numberOfDecimalDigits(latitude)),
        Math.pow(10, -numberOfDecimalDigits(longitude))
    );
  }

  function numberOfDecimalDigits(number) {
    var parts = number.split('.');
    if (parts.length < 2) {
      return 0;
    }
    return parts[1].length;
  }

  // "http://research.google.com/pubs/vrandecic.html"
  function isUrl(url) {
    if (typeof URL !== 'function') {
      return url.indexOf('http') === 0; // TODO: very bad fallback hack
    }

    try {
      url = new URL(url.toString());
      return url.protocol.indexOf('http') === 0 && url.host;
    } catch (e) {
      return false;
    }
  }

  function tsvValueToJson(value) {
    // From https://www.wikidata.org/wiki/Special:ListDatatypes and
    // https://de.wikipedia.org/wiki/Wikipedia:Wikidata/Wikidata_Spielwiese
    // https://www.wikidata.org/wiki/Special:EntityData/Q90.json

    // Q1
    var itemRegEx = /^Q\d+$/;

    // P1
    var propertyRegEx = /^P\d+$/;

    // @43.3111/-16.6655
    var coordinatesRegEx = /^@([+\-]?\d+(?:.\d+)?)\/([+\-]?\d+(?:.\d+))?$/;

    // fr:"Les Misérables"
    var languageStringRegEx = /^(\w+):("[^"\\]*(?:\\.[^"\\]*)*")$/;

    // +2013-01-01T00:00:00Z/10
    /* jshint maxlen: false */
    var timeRegEx = /^[+-]\d+-\d\d-\d\dT\d\d:\d\d:\d\dZ\/\d+$/;
    /* jshint maxlen: 80 */

    // +/-1234.4567
    var quantityRegEx = /^[+-]\d+(\.\d+)?$/;

    if (itemRegEx.test(value)) {
      return {
        type: 'wikibase-item',
        value: {
          'entity-type': 'item',
          'numeric-id': parseInt(value.replace(/^Q/, ''))
        }
      };
    } else if (propertyRegEx.test(value)) {
      return {
        type: 'wikibase-property',
        value: {
          'entity-type': 'property',
          'numeric-id': parseInt(value.replace(/^P/, ''))
        }
      };
    } else if (coordinatesRegEx.test(value)) {
      var latitude = value.replace(coordinatesRegEx, '$1');
      var longitude = value.replace(coordinatesRegEx, '$2');
      return {
        type: 'globe-coordinate',
        value: {
          latitude: parseFloat(latitude),
          longitude: parseFloat(longitude),
          altitude: null,
          precision: computeCoordinatesPrecision(latitude, longitude),
          globe: 'http://www.wikidata.org/entity/Q2'
        }
      };
    } else if (languageStringRegEx.test(value)) {
      return {
        type: 'monolingualtext',
        value: {
          language: value.replace(languageStringRegEx, '$1'),
          text: JSON.parse(value.replace(languageStringRegEx, '$2'))
        }
      };
    } else if (timeRegEx.test(value)) {
      var parts = value.split('/');
      return {
        type: 'time',
        value: {
          time: parts[0],
          timezone: 0,
          before: 0,
          after: 0,
          precision: parseInt(parts[1]),
          calendarmodel: 'http://www.wikidata.org/entity/Q1985727'
        }
      };
    } else if (quantityRegEx.test(value)) {
      return {
        type: 'quantity',
        value: {
          amount: value,
          unit: '1'
        }
      };
    } else {
      try {
        value = JSON.parse(value);
      } catch(e) { //If it is an invalid JSON we assume it is the value
      	if (!(e instanceof SyntaxError)) {
      		throw e;
      	}
      }
      if (isUrl(value)) {
        return {
          type: 'url',
          value: normalizeUrl(value)
        };
      } else {
        return {
          type: 'string',
          value: value
        };
      }
    }
  }

  function matchClaims(wikidataClaims, freebaseClaims) {
    var existingClaims = {};
    var newClaims = {};
    for (var property in freebaseClaims) {
      if (wikidataClaims[property]) {
        existingClaims[property] = freebaseClaims[property];
        var propertyLinks =
            document.querySelectorAll('a[title="Property:' + property + '"]');
        [].forEach.call(propertyLinks, function(propertyLink) {
          propertyLink.parentNode.parentNode.classList
              .add('existing-property');
        });
        for (var freebaseKey in freebaseClaims[property]) {
          var freebaseObject = freebaseClaims[property][freebaseKey];
          var existingWikidataObjects = {};
          var lenI = wikidataClaims[property].length;
          for (var i = 0; i < lenI; i++) {
            var wikidataObject = wikidataClaims[property][i];
            buildValueKeysFromWikidataStatement(wikidataObject)
              .forEach(function(key) {
                existingWikidataObjects[key] = wikidataObject;
              });
          }
          if (existingWikidataObjects[freebaseKey]) {
            // Existing object
            if (freebaseObject.sources.length === 0) {
              // No source, duplicate statement
              setStatementState(freebaseObject.id, STATEMENT_STATES.duplicate)
              .done(function() {
                debug.log('Automatically duplicate statement ' +
                    freebaseObject.id);
              });
            } else {
              // maybe new sources
              prepareNewSources(
                  property,
                  freebaseObject,
                  existingWikidataObjects[freebaseKey]
              );
            }
          } else {
            // New object
            var isDuplicate = false;
            for (var c = 0; c < wikidataClaims[property].length; c++) {
              var wikidataObject = wikidataClaims[property][c];

              if (wikidataObject.mainsnak.snaktype === 'value' &&
                  jsonToTsvValue(wikidataObject.mainsnak.datavalue) === freebaseObject.object) {
                isDuplicate = true;
                debug.log('Duplicate found! ' + property + ':' + freebaseObject.object);

                // Add new sources to existing statement
                prepareNewSources(
                    property,
                    freebaseObject,
                    wikidataObject
                );
              }
            }

            if (!isDuplicate) {
              createNewStatement(property, freebaseObject);
            }
          }
        }
      } else {
        newClaims[property] = freebaseClaims[property];
      }
    }
    for (var property in newClaims) {
      var claims = newClaims[property];
      debug.log('New claim ' + property);
      createNewClaim(property, claims);
    }
  }

  function buildValueKeysFromWikidataStatement(statement) {
    var mainSnak = statement.mainsnak;
    if (mainSnak.snaktype !== 'value') {
      return [mainSnak.snaktype];
    }

    var keys = [jsonToTsvValue(mainSnak.datavalue, mainSnak.datatype)];

    if (statement.qualifiers) {
      var qualifierKeyParts = [];
      $.each(statement.qualifiers, function(_, qualifiers) {
        qualifiers.forEach(function(qualifier) {
          qualifierKeyParts.push(
              qualifier.property + '\t' +
                  jsonToTsvValue(qualifier.datavalue, qualifier.datatype)
          );
        });
      });
      qualifierKeyParts.sort();
      keys.push(keys[0] + '\t' + qualifierKeyParts.join('\t'));
    }

    return keys;
  }

  function jsonToTsvValue(dataValue, dataType) {
    if (!dataValue.type) {
      debug.log('No data value type given');
      return dataValue.value;
    }
    switch (dataValue.type) {
    case 'quantity':
      return dataValue.value.amount;
    case 'time':
      var time = dataValue.value.time;

      // Normalize the timestamp
      if (dataValue.value.precision < 11) {
        time = time.replace('-01T', '-00T');
      }
      if (dataValue.value.precision < 10) {
        time = time.replace('-01-', '-00-');
      }

      return time + '/' + dataValue.value.precision;
    case 'globecoordinate':
      return '@' + dataValue.value.latitude + '/' + dataValue.value.longitude;
    case 'monolingualtext':
      return dataValue.value.language + ':' + JSON.stringify(dataValue.value.text);
    case 'string':
      var str = (dataType === 'url') ? normalizeUrl(dataValue.value)
                                     : dataValue.value;
      return JSON.stringify(str);
    case 'wikibase-entityid':
      switch (dataValue.value['entity-type']) {
        case 'item':
          return 'Q' + dataValue.value['numeric-id'];
        case 'property':
          return 'P' + dataValue.value['numeric-id'];
      }
    }
    debug.log('Unknown data value type ' + dataValue.type);
    return dataValue.value;
  }

  function normalizeUrl(url) {
    try {
      return (new URL(url.toString())).href;
    } catch (e) {
      return url;
    }
  }

  function prepareNewWikidataStatement(property, object) {
    getQualifiersAndSourcesLabels(object.qualifiers, object.sources)
    .done(function(labels) {
      object.sources.forEach(function(source) {
        source.forEach(function(snak) {
          snak.sourcePropertyLabel = labels[snak.sourceProperty];
        });
      });
      object.qualifiers.forEach(function(qualifier) {
        var qualifierProperty = qualifier.qualifierProperty;
        qualifier.qualifierPropertyLabel = labels[qualifierProperty];
      });

      return createNewStatement(property, object);
    });
  }

  function prepareNewSources(property, object, wikidataStatement) {
    var wikidataSources = ('references' in wikidataStatement) ?
                wikidataStatement.references :
                [];
    var existingSources = {};
    for (var i in wikidataSources) {
      var snakBag = wikidataSources[i].snaks;
      for (var prop in snakBag) {
        if (!(prop in existingSources)) {
          existingSources[prop] = {};
        }
        for (var j in snakBag[prop]) {
          var snak = snakBag[prop][j];
          if (snak.snaktype === 'value') {
            existingSources[prop]
                [jsonToTsvValue(snak.datavalue, snak.datatype)] = true;
          }
        }
      }
    }
    // Filter already present sources
    object.sources = object.sources.filter(function(source) {
      return source.filter(function(snak) {
        return !existingSources[snak.sourceProperty] ||
        !existingSources[snak.sourceProperty][snak.sourceObject];
      }).length > 0;
    });

    return createNewSources(
      object.sources,
      property,
      object,
      wikidataStatement.id
    );
  }

  function createNewSources(sources, property, object, statementId) {
    getSourcesHtml(sources, property, object).then(function(html) {
      var fragment = document.createDocumentFragment();
      var child = document.createElement('div');
      child.innerHTML = html;
      fragment.appendChild(child);
      // Need to find the correct reference
      var container = document
          .getElementsByClassName('wikibase-statement-' + statementId)[0];
      // Open the references toggle
      var toggler = container.querySelector('a.ui-toggler');
      if (toggler.classList.contains('ui-toggler-toggle-collapsed')) {
        toggler.click();
      }
      var label = toggler.querySelector('.ui-toggler-label');
      var oldLabel =
          parseInt(label.textContent.replace(/.*?(\d+).*?/, '$1'), 10);
      // Update the label
      var newLabel = oldLabel += sources.length;
      newLabel = newLabel === 1 ? '1 reference' : newLabel + ' references';
      label.textContent = newLabel;
      // Append the references
      container = container
          .querySelector('.wikibase-statementview-references');
      // Create wikibase-listview if not found
      if (!container.querySelector('.wikibase-listview')) {
        var sourcesListView = document.createElement('div');
        sourcesListView.className = 'wikibase-listview';
        container.insertBefore(sourcesListView, container.firstChild);
      }
      container = container.querySelector('.wikibase-listview');
      container.appendChild(fragment);
    });
  }

  function createNewStatement(property, object) {
    getStatementHtml(property, object).then(function(html) {
      var fragment = document.createDocumentFragment();
      var child = document.createElement('div');
      child.innerHTML = html;
      fragment.appendChild(child.firstChild);
      var container = document.getElementById(property)
          .querySelector('.wikibase-statementlistview-listview');
      container.appendChild(fragment);
      appendToNav(document.getElementById(property));
    });
  }

  function createNewClaim(property, claims) {
    var newClaim = {
      property: property,
      objects: []
    };
    var objectsLength = Object.keys(claims).length;
    var i = 0;
    for (var key in claims) {
      var object = claims[key].object;
      var id = claims[key].id;
      var sources = claims[key].sources;
      var qualifiers = claims[key].qualifiers;
      newClaim.objects.push({
        object: object,
        id: id,
        qualifiers: qualifiers,
        sources: sources,
        key: key
      });
      (function(currentNewClaim, currentKey) {
        currentNewClaim.objects.forEach(function(object) {
          if (object.key !== currentKey) {
            return;
          }
          i++;
          if (i === objectsLength) {
            return createNewClaimList(currentNewClaim);
          }
        });
      })(newClaim, key);
    }
  }

  function createNewClaimList(newClaim) {
    var container = document
        .querySelector('.wikibase-statementgrouplistview')
        .querySelector('.wikibase-listview');
    var statementPromises = newClaim.objects.map(function(object) {
      return getStatementHtml(newClaim.property, object);
    });

    getValueHtml(newClaim.property).done(function(propertyHtml) {
      $.when.apply($, statementPromises).then(function() {
        var statementViewsHtml = Array.prototype.slice.call(arguments).join('');
        var mainHtml = HTML_TEMPLATES.mainHtml
            .replace(/\{\{statement-views\}\}/g, statementViewsHtml)
            .replace(/\{\{property\}\}/g, newClaim.property)
            .replace(/\{\{data-property\}\}/g, newClaim.property)
            .replace(/\{\{property-html\}\}/g, propertyHtml);

        var fragment = document.createDocumentFragment();
        var child = document.createElement('div');
        child.innerHTML = mainHtml;
        fragment.appendChild(child.firstChild);
        container.appendChild(fragment);
        appendToNav(container.lastChild);
      });
    });
  }

  function escapeHtml(html) {
    return html
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/\"/g, '&quot;');
  }

  var valueHtmlCache = {};
  function getValueHtml(value, property) {
    var cacheKey = property + '\t' + value;
    if (cacheKey in valueHtmlCache) {
      return valueHtmlCache[cacheKey];
    }
    var parsed = tsvValueToJson(value);
    var dataValue = {
        type: getValueTypeFromDataValueType(parsed.type),
        value: parsed.value
      };
    var options = {
        'lang': mw.language.getFallbackLanguageChain()[0] || 'en'
      };

    if (parsed.type === 'string') { // Link to external database
      valueHtmlCache[cacheKey] = getUrlFormatter(property)
      .then(function(urlFormatter) {
        if (urlFormatter === '') {
          return parsed.value;
        } else {
          var url = urlFormatter.replace('$1', parsed.value);
          return '<a rel="nofollow" class="external free" href="' + url + '">' +
                 parsed.value + '</a>';
        }
      });
    } else if (parsed.type === 'url') {
      valueHtmlCache[cacheKey] = $.Deferred().resolve(
          '<a rel="nofollow" class="external free" href="' + parsed.value + '">' + parsed.value + '</a>'
      );
    } else if(parsed.type === 'wikibase-item' || parsed.type === 'wikibase-property') {
      return getEntityLabel(value).then(function(label) {
        return '<a href="/entity/' + value + '">' + label + '</a>'; //TODO: better URL
      });
    } else {
      var api = new mw.Api();
      valueHtmlCache[cacheKey] = api.get({
        action: 'wbformatvalue',
        generate: 'text/html',
        datavalue: JSON.stringify(dataValue),
        datatype: parsed.type,
        options: JSON.stringify(options)
      }).then(function(result) {
        // Create links for geocoordinates
        if (parsed.type === 'globe-coordinate') {
          var url = 'https://tools.wmflabs.org/geohack/geohack.php' +
              '?language=' + mw.config.get('wgUserLanguage') + '&params=' +
              dataValue.value.latitude + '_N_' +
              dataValue.value.longitude + '_E_globe:earth';
          return '<a rel="nofollow" class="external free" href="' + url + '">' +
              result.result + '</a>';
        }

        return result.result;
      });
    }

    return valueHtmlCache[cacheKey];
  }

  var urlFormatterCache = {};
  function getUrlFormatter(property) {
    if (property in urlFormatterCache) {
      return urlFormatterCache[property];
    }

    var api = new mw.Api();
    urlFormatterCache[property] = api.get({
      action: 'wbgetentities',
      ids: property,
      props: 'claims'
    }).then(function(result) {
      var urlFormatter = '';
      $.each(result.entities, function(_, entity) {
        if (entity.claims && 'P1630' in entity.claims) {
          urlFormatter = entity.claims.P1630[0].mainsnak.datavalue.value;
        }
      });
      return urlFormatter;
    });
    return urlFormatterCache[property];
  }

  var entityLabelCache = {};
  function getEntityLabel(entityId) {
    if(!(entityId in entityLabelCache)) {
      loadEntityLabels([entityId]);
    }

    return entityLabelCache[entityId];
  }

  function loadEntityLabels(entityIds) {
    entityIds = entityIds.filter(function(entityId) {
      return !(entityId in entityLabelCache);
    });
    if(entityIds.length === 0) {
      return;
    }

    var promise = getEntityLabels(entityIds);
    entityIds.forEach(function(entityId) {
      entityLabelCache[entityId] = promise.then(function(labels) {
        return labels[entityId];
      });
    });
  }

  function preloadEntityLabels(statements) {
    var entityIds = [];
    statements.forEach(function(statement) {
      entityIds = entityIds.concat(extractEntityIdsFromStatement(statement));
    });
    loadEntityLabels(entityIds);
  }

  function extractEntityIdsFromStatement(statement) {
    function isEntityId(str) {
      return /^[PQ]\d+$/.test(str);
    }

    var entityIds = [statement.subject, statement.predicate];

    if (isEntityId(statement.object)) {
      entityIds.push(statement.object);
    }

    statement.qualifiers.forEach(function(qualifier) {
      entityIds.push(qualifier.qualifierProperty);
      if(isEntityId(qualifier.qualifierObject)) {
        entityIds.push(qualifier.qualifierObject);
      }
    });

    statement.source.forEach(function(snak) {
      entityIds.push(snak.sourceProperty);
      if(isEntityId(snak.sourceObject)) {
        entityIds.push(snak.sourceObject);
      }
    });

    return entityIds;
  }

  function getSourcesHtml(sources, property, object) {
    var sourcePromises = sources.map(function(source) {
      var sourceItemsPromises = source.map(function(snak) {
        return $.when(
            getValueHtml(snak.sourceProperty),
            getValueHtml(snak.sourceObject, snak.sourceProperty)
        ).then(function(formattedProperty, formattedValue) {
          return HTML_TEMPLATES.sourceItemHtml
            .replace(/\{\{source-property-html\}\}/g, formattedProperty)
            .replace(/\{\{source-object\}\}/g, formattedValue);
        });
      });

      return $.when.apply($, sourceItemsPromises).then(function() {
        return HTML_TEMPLATES.sourceHtml
          .replace(/\{\{data-source\}\}/g, escapeHtml(JSON.stringify(source)))
          .replace(/\{\{data-property\}\}/g, property)
          .replace(/\{\{data-object\}\}/g, escapeHtml(object.object))
          .replace(/\{\{statement-id\}\}/g, source[0].sourceId)
          .replace(/\{\{source-html\}\}/g,
              Array.prototype.slice.call(arguments).join(''))
          .replace(/\{\{data-qualifiers\}\}/g, escapeHtml(JSON.stringify(
              object.qualifiers)));
      });
    });

    return $.when.apply($, sourcePromises).then(function() {
      return Array.prototype.slice.call(arguments).join('');
    });
  }

  function getQualifiersHtml(qualifiers) {
    var qualifierPromises = qualifiers.map(function(qualifier) {
      return $.when(
        getValueHtml(qualifier.qualifierProperty),
        getValueHtml(qualifier.qualifierObject, qualifier.qualifierProperty)
      ).then(function(formattedProperty, formattedValue) {
        return HTML_TEMPLATES.qualifierHtml
          .replace(/\{\{qualifier-property-html\}\}/g, formattedProperty)
          .replace(/\{\{qualifier-object\}\}/g, formattedValue);
      });
    });

    return $.when.apply($, qualifierPromises).then(function() {
      return Array.prototype.slice.call(arguments).join('');
    });
  }

  function getStatementHtml(property, object) {
    return $.when(
        getQualifiersHtml(object.qualifiers),
        getSourcesHtml(object.sources, property, object),
        getValueHtml(object.object, property)
    ).then(function(qualifiersHtml, sourcesHtml, formattedValue) {
      return HTML_TEMPLATES.statementViewHtml
        .replace(/\{\{object\}\}/g, formattedValue)
        .replace(/\{\{data-object\}\}/g, escapeHtml(object.object))
        .replace(/\{\{data-property\}\}/g, property)
        .replace(/\{\{references\}\}/g,
          object.sources.length === 1 ?
              object.sources.length + ' reference' :
              object.sources.length + ' references')
        .replace(/\{\{sources\}\}/g, sourcesHtml)
        .replace(/\{\{qualifiers\}\}/g, qualifiersHtml)
        .replace(/\{\{statement-id\}\}/g, object.id)
        .replace(/\{\{data-qualifiers\}\}/g, escapeHtml(JSON.stringify(
            object.qualifiers)))
        .replace(/\{\{data-sources\}\}/g, escapeHtml(JSON.stringify(
            object.sources)));
    });
  }

  function getWikidataEntityData(qid, callback) {
    $.ajax({
      url: WIKIDATA_ENTITY_DATA_URL.replace(/\{\{qid\}\}/, qid) + '?revision=' +
          mw.config.get('wgRevisionId')
    }).done(function(data) {
      return callback(null, data.entities[qid]);
    }).fail(function() {
      return callback('Invalid revision ID ' + mw.config.get('wgRevisionId'));
    });
  }

  function getFreebaseEntityData(qid, callback) {
    $.ajax({
      url: FAKE_OR_RANDOM_DATA ?
          FREEBASE_ENTITY_DATA_URL.replace(/\{\{qid\}\}/, 'any') :
          FREEBASE_ENTITY_DATA_URL.replace(/\{\{qid\}\}/, qid) + '?dataset=' +
          dataset
    }).done(function(data) {
      return callback(null, data);
    });
  }


  function getEntityLabels(entityIds) {
    //Split entityIds per bucket in order to match limits
    var buckets = [];
    var currentBucket = [];

    entityIds.forEach(function(entityId) {
      currentBucket.push(entityId);
      if(currentBucket.length > 40) {
        buckets.push(currentBucket);
        currentBucket = [];
      }
    });
    buckets.push(currentBucket);

    var promises = buckets.map(function(bucket) {
      return getFewEntityLabels(bucket);
    });

    return $.when.apply(this, promises).then(function() {
      return $.extend.apply(this, arguments);
    });
  }

  function getFewEntityLabels(entityIds) {
    if (entityIds.length === 0) {
      return $.Deferred().resolve({});
    }
    var api = new mw.Api();
    var language = mw.config.get('wgUserLanguage');
    return api.get({
      action: 'wbgetentities',
      ids: entityIds.join('|'),
      props: 'labels',
      languages: language,
      languagefallback: true
    }).then(function(data) {
      var labels = {};
      for (var id in data.entities) {
        var entity = data.entities[id];
        if (entity.labels && entity.labels[language]) {
          labels[id] = entity.labels[language].value;
        } else {
          labels[id] = entity.id;
        }
      }
      return labels;
    });
  }

  function setStatementState(id, state) {
    if (!STATEMENT_STATES[state]) {
      reportError('Invalid statement state');
    }
    var user = mw.user.getName();
    var url = FREEBASE_STATEMENT_APPROVAL_URL
        .replace(/\{\{user\}\}/, user)
        .replace(/\{\{state\}\}/, state)
        .replace(/\{\{id\}\}/, id);

    return $.post(url).fail(function() {
      reportError('Set statement state to ' + state + ' failed.');
    });
  }

  // https://www.wikidata.org/w/api.php?action=help&modules=wbcreateclaim
  function createClaim(subject, predicate, object, qualifiers) {
    var value = (tsvValueToJson(object)).value;
    var api = new mw.Api();
    return api.postWithToken('csrf', {
      action: 'wbcreateclaim',
      entity: subject,
      property: predicate,
      snaktype: 'value',
      value: JSON.stringify(value),
      summary: WIKIDATA_API_COMMENT
    }).then(function(data) {
      // We save the qualifiers sequentially in order to avoid edit conflict
      var saveQualifiers = function() {
        var qualifier = qualifiers.pop();
        if (qualifier === undefined) {
          return data;
        }

        var value = (tsvValueToJson(qualifier.qualifierObject)).value;
        return api.postWithToken('csrf', {
          action: 'wbsetqualifier',
          claim: data.claim.id,
          property: qualifier.qualifierProperty,
          snaktype: 'value',
          value: JSON.stringify(value),
          summary: WIKIDATA_API_COMMENT
        }).then(saveQualifiers);
      };

      return saveQualifiers();
    });
  }

  // https://www.wikidata.org/w/api.php?action=help&modules=wbsetreference
  function createClaimWithReference(subject, predicate, object,
      qualifiers, sourceSnaks) {
    var api = new mw.Api();
    return createClaim(subject, predicate, object, qualifiers)
    .then(function(data) {
      return api.postWithToken('csrf', {
        action: 'wbsetreference',
        statement: data.claim.id,
        snaks: JSON.stringify(formatSourceForSave(sourceSnaks)),
        summary: WIKIDATA_API_COMMENT
      });
    });
  }

  // https://www.wikidata.org/w/api.php?action=help&modules=wbgetclaims
  function getClaims(subject, predicate, callback) {
    var api = new mw.Api();
    api.get({
      action: 'wbgetclaims',
      entity: subject,
      property: predicate
    }).done(function(data) {
      return callback(null, data.claims[predicate] || []);
    }).fail(function(error) {
      return callback(error);
    });
  }

  // https://www.wikidata.org/w/api.php?action=help&modules=wbsetreference
  function createReference(subject, predicate, object, sourceSnaks, callback) {
    var api = new mw.Api();
    api.get({
      action: 'wbgetclaims',
      entity: subject,
      property: predicate
    }).then(function(data) {
      var index = -1;
      for (var i = 0, lenI = data.claims[predicate].length; i < lenI; i++) {
        var claimObject = data.claims[predicate][i];
        var mainSnak = claimObject.mainsnak;
        if (
            mainSnak.snaktype === 'value' &&
            jsonToTsvValue(mainSnak.datavalue, mainSnak.datatype) === object
        ) {
          index = i;
          break;
        }
      }
      return api.postWithToken('csrf', {
        action: 'wbsetreference',
        statement: data.claims[predicate][index].id,
        snaks: JSON.stringify(formatSourceForSave(sourceSnaks)),
        summary: WIKIDATA_API_COMMENT
      });
    }).done(function(data) {
      return callback(null, data);
    }).fail(function(error) {
      return callback(error);
    });
  }

  function formatSourceForSave(sourceSnaks) {
    var result = {};
    sourceSnaks.forEach(function(snak) {
      result[snak.sourceProperty] = [];
    });

    sourceSnaks.forEach(function(snak) {
      var dataValue = tsvValueToJson(snak.sourceObject);
      var type = getValueTypeFromDataValueType(dataValue.type);

      result[snak.sourceProperty].push({
        snaktype: 'value',
        property: snak.sourceProperty,
        datavalue: {
          type: type,
          value: dataValue.value
        }
      });
    });

    return result;
  }

  function getValueTypeFromDataValueType(dataValueType) {
    return wikibase.dataTypeStore.getDataType(dataValueType)
        .getDataValueType();
  }

  function getBlacklistedSourceUrls() {
    var now = Date.now();
    if (localStorage.getItem('f2w_blacklist')) {
      var blacklist = JSON.parse(localStorage.getItem('f2w_blacklist'));
      if (!blacklist.timestamp) {
        blacklist.timestamp = 0;
      }
      if (now - blacklist.timestamp < CACHE_EXPIRY) {
        debug.log('Using cached source URL blacklist');
        return $.Deferred().resolve(blacklist.data);
      }
    }
    return $.ajax({
      url: FREEBASE_SOURCE_URL_BLACKLIST
    }).then(function(data) {
      if (data && data.parse && data.parse.text && data.parse.text['*']) {
        var blacklist = data.parse.text['*']
            .replace(/\n/g, '')
            .replace(/^.*?<ul>(.*?)<\/ul>.*?$/g, '$1')
            .replace(/<\/li>/g, '')
            .split('<li>').slice(1)
            .map(function(url) {
              return url.trim();
            })
            .filter(function(url) {
              var copy = url;
              if (/\s/g.test(copy) || !/\./g.test(copy)) {
                return false;
              }
              if (!/^https?:\/\//.test(copy)) {
                copy = 'http://' + url;
              }
              try {
                return (new URL(copy)).host !== '';
              } catch (e) {
                return false;
              }
            });
        debug.log('Caching source URL blacklist');
        localStorage.setItem('f2w_blacklist', JSON.stringify({
          timestamp: now,
          data: blacklist
        }));
        return blacklist;
      } else {
        // Fail silently
        debug.log('Could not obtain blacklisted source URLs');
        return [];
      }
    });
  }

  function getWhitelistedSourceUrls() {
    var now = Date.now();
    if (localStorage.getItem('f2w_whitelist')) {
      var whitelist = JSON.parse(localStorage.getItem('f2w_whitelist'));
      if (!whitelist.timestamp) {
        whitelist.timestamp = 0;
      }
      if (now - whitelist.timestamp < CACHE_EXPIRY) {
        debug.log('Using cached source URL whitelist');
        return $.Deferred().resolve(whitelist.data);
      }
    }
    return $.ajax({
      url: FREEBASE_SOURCE_URL_WHITELIST
    }).then(function(data) {
      if (data && data.parse && data.parse.text && data.parse.text['*']) {
        var whitelist = data.parse.text['*']
            .replace(/\n/g, '')
            .replace(/^.*?<ul>(.*?)<\/ul>.*?$/g, '$1')
            .replace(/<\/li>/g, '')
            .split('<li>').slice(1)
            .map(function(url) {
              return url.trim();
            })
            .filter(function(url) {
              var copy = url;
              if (/\s/g.test(copy) || !/\./g.test(copy)) {
                return false;
              }
              if (!/^https?:\/\//.test(copy)) {
                copy = 'http://' + url;
              }
              try {
                return (new URL(copy)).host !== '';
              } catch (e) {
                return false;
              }
            });
        debug.log('Caching source URL whitelist');
        localStorage.setItem('f2w_whitelist', JSON.stringify({
          timestamp: now,
          data: whitelist
        }));
        return whitelist;
      } else {
        // Fail silently
        debug.log('Could not obtain whitelisted source URLs');
        return [];
      }
    });
  }

  function getBlacklistedSourceUrlsWithCallback(callback) {
    getBlacklistedSourceUrls()
    .done(function(blacklist) {
      callback(null, blacklist);
    })
    .fail(function() {
      debug.log('Could not obtain blacklisted source URLs');
      callback(null);
    });
  }

  function getWhitelistedSourceUrlsWithCallback(callback) {
    getWhitelistedSourceUrls()
    .done(function(whitelist) {
      callback(null, whitelist);
    })
    .fail(function() {
      debug.log('Could not obtain whitelisted source URLs');
      callback(null);
    });
  }

  function searchStatements(parameters) {
    return $.when(
      $.ajax({
        url: FREEBASE_STATEMENT_SEARCH_URL,
        data: parameters
      }).then(function(data) { return data; }),
      getBlacklistedSourceUrls()
   ).then(function(data, blacklistedSourceUrls) {
      var isBlacklisted = isBlackListedBuilder(blacklistedSourceUrls);
      var statements = data.map(function(statement) {
        return parsePrimarySourcesStatement(statement, isBlacklisted);
      });
      preloadEntityLabels(statements);
      return statements;
    });
  }

  function listDialog(button) {

    /**
     * A row displaying a statement
     *
     * @class
     * @extends OO.ui.Widget
     * @cfg {Object} [statement] the statement to display
     */
    function StatementRow(config) {
      StatementRow.super.call(this, config);

      this.statement = config.statement;
      var widget = this;
      var numberOfSnaks = this.statement.qualifiers.length + 1;

      var htmlCallbacks = [
        getValueHtml(this.statement.subject),
        getValueHtml(this.statement.predicate),
        getValueHtml(this.statement.object, this.statement.predicate)
      ];
      this.statement.qualifiers.forEach(function(qualifier) {
        htmlCallbacks.push(getValueHtml(qualifier.qualifierProperty));
        htmlCallbacks.push(
          getValueHtml(qualifier.qualifierObject, qualifier.qualifierProperty)
        );
      });

      $.when.apply(this, htmlCallbacks).then(function() {
        var subjectHtml = arguments[0];
        var propertyHtml = arguments[1];
        var objectHtml = arguments[2];

        var approveButton = new OO.ui.ButtonWidget({
          label: 'Approve',
          flags: 'constructive'
        });
        approveButton.connect(widget, {click: 'approve'});

        var rejectButton = new OO.ui.ButtonWidget({
          label: 'Reject',
          flags: 'destructive'
        });
        rejectButton.connect(widget, {click: 'reject'});

        var buttonGroup = new OO.ui.ButtonGroupWidget({
          items: [approveButton, rejectButton]
        });

        // Main row
        widget.$element
          .attr('data-id', widget.statement.id)
          .append(
            $('<tr>').append(
              $('<td>')
                .attr('rowspan', numberOfSnaks)
                .html(subjectHtml),
              $('<td>')
                .attr('rowspan', numberOfSnaks)
                .html(propertyHtml),
              $('<td>')
                .attr('colspan', 2)
                .html(objectHtml),
              $('<td>')
                .attr('rowspan', numberOfSnaks)
                .append(buttonGroup.$element)
            )
          );

        // Qualifiers
        for (var i = 3; i < arguments.length; i += 2) {
          widget.$element.append(
            $('<tr>').append(
              $('<td>').html(arguments[i]),
              $('<td>').html(arguments[i + 1])
            )
          );
        }

        // Check that the statement don't already exist
        getClaims(widget.statement.subject, widget.statement.predicate,
          function(err, statements) {
            for (var i in statements) {
              buildValueKeysFromWikidataStatement(statements[i]);
              if ($.inArray(
                widget.statement.key,
                buildValueKeysFromWikidataStatement(statements[i])
              ) !== -1) {
                widget.toggle(false).setDisabled(true);
                if (widget.statement.source.length === 0) {
                  setStatementState(widget.statement.id,
                      STATEMENT_STATES.duplicate).done(function() {
                    debug.log(widget.statement.id + ' tagged as duplicate');
                  });
                }
              }
            }
          });
      });
    }
    OO.inheritClass(StatementRow, OO.ui.Widget);
    StatementRow.static.tagName = 'tbody';

    StatementRow.prototype.approve = function() {
      var widget = this;

      this.showProgressBar();
      createClaim(
        this.statement.subject,
        this.statement.predicate,
        this.statement.object,
        this.statement.qualifiers
      ).fail(function(error) {
        return reportError(error);
      }).done(function() {
        if (this.statement.source.length > 0) {
          return; // TODO add support of source review
        }
        setStatementState(widget.statement.id, STATEMENT_STATES.approved)
        .done(function() {
          widget.toggle(false).setDisabled(true);
        });
      });
    };

    StatementRow.prototype.reject = function() {
      var widget = this;

      this.showProgressBar();
      setStatementState(widget.statement.id, STATEMENT_STATES.rejected)
      .done(function() {
        widget.toggle(false).setDisabled(true);
      });
    };

    StatementRow.prototype.showProgressBar = function() {
      var progressBar = new OO.ui.ProgressBarWidget();
      progressBar.$element.css('max-width', '100%');
      this.$element.empty()
        .append(
          $('<td>')
            .attr('colspan', 4)
            .append(progressBar.$element)
        );
    };

    /**
     * The main dialog
     *
     * @class
     * @extends OO.ui.Widget
     */
    function ListDialog(config) {
      ListDialog.super.call(this, config);
    }
    OO.inheritClass(ListDialog, OO.ui.ProcessDialog);
    ListDialog.static.name = 'ps-list';
    ListDialog.static.title = 'Primary Sources statement list (in development)';
    ListDialog.static.size = 'larger';
    ListDialog.static.actions = [
      {label: 'Close', flags: 'safe'}
    ];

    ListDialog.prototype.initialize = function() {
      ListDialog.super.prototype.initialize.apply(this, arguments);

      var widget = this;

      // Selection form
      this.datasetInput = new OO.ui.DropdownInputWidget();
      getPossibleDatasets(function(datasets) {
        var options = [{data: '', label: 'All sources'}];
        for (var datasetId in datasets) {
          options.push({data: datasetId, label: datasetId});
        }
        widget.datasetInput.setOptions(options)
                    .setValue(dataset);
      });

      this.propertyInput = new OO.ui.TextInputWidget({
        placeholder: 'PXX',
        validate: /^[pP]\d+$/
      });

      this.valueInput = new OO.ui.TextInputWidget({
        placeholder: 'Filter by value like item id'
      });

      var loadButton = new OO.ui.ButtonInputWidget({
        label: 'Load',
        flags: 'progressive',
        type: 'submit'
      });
      loadButton.connect(this, {click: 'onOptionSubmit'});

      var fieldset = new OO.ui.FieldsetLayout({
        label: 'Query options',
        classes: ['container']
      });
      fieldset.addItems([
        new OO.ui.FieldLayout(this.datasetInput, {label: 'Dataset'}),
        new OO.ui.FieldLayout(this.propertyInput, {label: 'Property'}),
        new OO.ui.FieldLayout(this.valueInput, {label: 'Value'}),
        new OO.ui.FieldLayout(loadButton)
      ]);
      var formPanel = new OO.ui.PanelLayout({
        padded: true,
        framed: true
      });
      formPanel.$element.append(fieldset.$element);

      // Main panel
      var alertIcon = new OO.ui.IconWidget({
        icon: 'alert'
      });
      var description = new OO.ui.LabelWidget({
        label: 'This feature is currently in active development. ' +
               'It allows to list statements contained in Primary Sources ' +
               'and do action on them. Statements with qualifiers or sources ' +
               'are hidden.'
      });
      this.mainPanel = new OO.ui.PanelLayout({
        padded: true,
        scrollable: true
      });
      this.mainPanel.$element.append(alertIcon.$element, description.$element);

      // Final layout
      this.stackLayout = new OO.ui.StackLayout({
        continuous: true
      });
      this.stackLayout.addItems([formPanel, this.mainPanel]);
      this.$body.append(this.stackLayout.$element);
    };

    ListDialog.prototype.onOptionSubmit = function() {
      this.mainPanel.$element.empty();
      this.table = null;
      this.parameters = {
          dataset: this.datasetInput.getValue(),
          property: this.propertyInput.getValue(),
          value: this.valueInput.getValue(),
          offset: 0,
          limit: 100
        };
      this.alreadyDisplayedStatementKeys = {};

      this.executeQuery();
    };

    ListDialog.prototype.executeQuery = function() {
      var widget = this;

      var progressBar = new OO.ui.ProgressBarWidget();
      progressBar.$element.css('max-width', '100%');
      widget.mainPanel.$element.append(progressBar.$element);

      searchStatements(this.parameters)
      .fail(function() {
        progressBar.$element.remove();
        var description = new OO.ui.LabelWidget({
          label: 'No statements found.'
        });
        widget.mainPanel.$element.append(description.$element);
      })
      .done(function(statements) {
        progressBar.$element.remove();

        widget.parameters.offset += widget.parameters.limit;
        widget.displayStatements(statements);

        // We may assume that more statements remains
        if (statements.length > 0) {
          widget.nextStatementsButton = new OO.ui.ButtonWidget({
            label: 'Load more statements',
          });
          widget.nextStatementsButton.connect(
            widget,
            {click: 'onNextButtonSubmit'}
          );
          widget.mainPanel.$element.append(
            widget.nextStatementsButton.$element
          );
        }
      });
    };

    ListDialog.prototype.onNextButtonSubmit = function() {
      this.nextStatementsButton.$element.remove();
      this.executeQuery();
    };

    ListDialog.prototype.displayStatements = function(statements) {
      var widget = this;

      if (this.table === null) { // Initialize the table
        this.initTable();
      }

      statements.map(function(statement) {
        statement.key = statement.subject + '\t' +
                        statement.predicate + '\t' +
                        statement.object;
        statement.qualifiers.forEach(function(qualifier) {
          statement.key += '\t' + qualifier.qualifierProperty + '\t' +
                                  qualifier.qualifierObject;
        });
        if (statement.key in widget.alreadyDisplayedStatementKeys) {
          return; // Don't display twice the same statement
        }
        widget.alreadyDisplayedStatementKeys[statement.key] = true;

        var row = new StatementRow({
          statement: statement
        });
        widget.table.append(row.$element);
      });
    };

    ListDialog.prototype.initTable = function() {
      this.table = $('<table>')
        .addClass('wikitable')
        .css('width', '100%')
        .append(
          $('<thead>').append(
            $('<tr>').append(
              $('<th>').text('Subject'),
              $('<th>').text('Property'),
              $('<th>').attr('colspan', 2).text('Object'),
              $('<th>').text('Action')
            )
          )
        );
      this.mainPanel.$element.append(this.table);
    };

    ListDialog.prototype.getBodyHeight = function() {
      return window.innerHeight - 100;
    };

    windowManager.addWindows([new ListDialog()]);

    button.click(function() {
      windowManager.openWindow('ps-list');
    });
  }
});
