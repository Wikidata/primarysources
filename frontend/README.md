# freebase2wikidata.js
Wikidata userscript for the migration of Freebase to Wikidata.

## Installation
### For users
On Wikidata you just have to activate the gadget "Primary Sources" in [your user preferences](https://www.wikidata.org/wiki/Special:Preferences#mw-prefsection-gadgets).

### For developpers
In order to be able to test the script without affecting the other users the best option is to:
* Add the content of `freebase2wikidata.css` into [your common.css](https://www.wikidata.org/wiki/Special:MyPage/common.css)
* Add the content of `freebase2wikidata.js` surrounded by these lines into [your common.js](https://www.wikidata.org/wiki/Special:MyPage/common.js):

```javascript
var asyncSrc = 'https://www.wikidata.org/w/index.php?title=' +
      'MediaWiki:Gadget-PrimarySources-async.js' +
      '&action=raw&ctype=text%2Fjavascript';
$.getScript(asyncSrc).done(function() {
//CONTENT OF THE FILE HERE
});
```

## Bug reports and feature requests
Please use the primarysources tool's GitHub [issues](https://github.com/google/primarysources/issues) or [pull requests](https://github.com/google/primarysources/pulls) features. 

## Happy migrating!
Thanks for making Wikidata even more awesome.

## Deployment on Wikidata
Requires admin rights on Wikidata.

* Copy the content of `freebase2wikidata.css` into https://www.wikidata.org/wiki/MediaWiki:Gadget-PrimarySources.css
* Copy the content of `freebase2wikidata.js` into https://www.wikidata.org/wiki/MediaWiki:Gadget-PrimarySources.js

To update the gadget description edit https://www.wikidata.org/wiki/MediaWiki:Gadget-PrimarySources