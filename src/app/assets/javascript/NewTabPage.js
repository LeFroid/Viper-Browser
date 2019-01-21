// ==UserScript==
// @include viper://newtab
// @exclude /^http/
// ==/UserScript==

var onWebChannelSetup = function(cb) {
    if (window._webchannel_initialized) {
        cb();
    } else {
        document.addEventListener("_webchannel_setup", cb);
    }
};
        
var getFavoritePages = function() {
    return new Promise(function(resolve) {
        onWebChannelSetup(() => {
            window.viper.favoritePageManager.getFavorites(function(result) {
                resolve(result);
            });
        });
    });
};

var pageList = [];
var nextItem = 0;

const cellTemplate = '<div class="cell" draggable="true"><span data-elemid="{{id}}" class="close">&times;</span><a href="{{url}}">'
            + '<img class="thumbnail" src="{{imgSrc}}" alt="{{title}}"><div class="titleContainer">'
            + '<div class="titleTextWrapper"><span data-elemid="{{id}}" class="title">{{title}}</span></div></div></a></div>';
const cellTemplateNoThumbnail = '<div class="cell" draggable="true"><span class="close">&times;</span><a href="{{url}}">'
            + '<div class="thumbnail thumbnailMock"></div><div class="titleContainer"><div class="titleTextWrapper">'
            + '<span class="title">{{title}}</span></div></div></a></div>';

// Allow for user to modify the cards on the page
document.addEventListener('click', function(e) {
    if (!e.target || pageList.length == 0)
        return;

    let target = e.target;
    if (!('dataset' in target) || !('elemid' in target.dataset))
        return;

    let elemId = target.dataset['elemid'];
    if (elemId >= pageList.length)
        return;

    // Remove item from favorite page collection
    let pageData = pageList[elemId];
    window.viper.favoritePageManager.removeEntry(pageData.url);

    // Remove item from web page
    let p = target.parentNode;
    p.parentNode.removeChild(p);

    // Attempt to add the next item to the web page
    if (nextItem < pageList.length) {
        let mainContainer = document.getElementById('mainGrid');
        let item = pageList[nextItem];
        if (item == null || !('url' in item) || !('title' in item) || !('thumbnail' in item))
            return;
        let itemHtml = (item.thumbnail == '') ? cellTemplateNoThumbnail : cellTemplate;
        itemHtml = itemHtml.replace(/{{id}}/g, nextItem)
                           .replace(/{{url}}/g, item.url)
                           .replace(/{{imgSrc}}/g, item.thumbnail)
                           .replace(/{{title}}/g, item.title);
        mainContainer.innerHTML += itemHtml;
        ++nextItem;
    }
});

getFavoritePages().then(function(result) {
    if (!result)
        return;
    
    pageList = result;

    var mainContainer = document.getElementById('mainGrid');
    const maxResults = result.length < 8 ? result.length : 8;
    for (var i = 0; i < maxResults; ++i) {
        nextItem = i + 1;
        var item = result[i];
        if (item == null || !('url' in item) || !('title' in item) || !('thumbnail' in item))
            continue;
        var itemHtml = (item.thumbnail == '') ? cellTemplateNoThumbnail : cellTemplate;
        itemHtml = itemHtml.replace(/{{id}}/g, i)
                           .replace(/{{url}}/g, item.url)
                           .replace(/{{imgSrc}}/g, item.thumbnail)
                           .replace(/{{title}}/g, item.title);
        mainContainer.innerHTML += itemHtml;
    }
});
