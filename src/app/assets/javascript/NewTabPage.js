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

const addPageElem = document.getElementById('addItem'),
    addPageDialog = document.getElementById('addPageDialog'),
    closeDialogElem = document.getElementById('closeDialog'),
    formAddPageElem = document.getElementById('formAddPage'),
    inputPageUrl = document.getElementById('inputUrl'),
    inputPageTitle = document.getElementById('inputTitle');

addPageElem.addEventListener('click', function(e) {
    e.preventDefault();
    
    let newStyle = 'block';
    let computedDisplay = window.getComputedStyle(addPageDialog).getPropertyValue('display');
    if (computedDisplay == 'block') {
        newStyle = 'none';
    } else {
        inputPageTitle.value = '';
        inputPageUrl.value = '';
    }
    
    addPageDialog.style.display = newStyle;
});

closeDialogElem.addEventListener('click', function(e) {
    e.preventDefault();
    addPageDialog.style.display = 'none';
});

const cellTemplate = '<div class="cell" draggable="true"><div class="closeContainer"><span data-elemid="{{id}}" class="close">&times;</span>'
            + '</div><div class="thumbnailContainer"><a href="{{url}}"><img class="thumbnail" src="{{imgSrc}}" alt="{{title}}">'
            + '<div class="titleContainer"><div class="titleTextWrapper"><span class="title">{{title}}</span></div></div></a></div></div>';
const cellTemplateNoThumbnail = '<div class="cell" draggable="true"><div class="closeContainer">'
            + '<span data-elemid="{{id}}" class="close">&times;</span></div><div class="thumbnailContainer"><a href="{{url}}">'
            + '<div class="thumbnail thumbnailMock"></div><div class="titleContainer"><div class="titleTextWrapper">'
            + '<span class="title">{{title}}</span></div></div></a></div></div>';

// Callback when user pins a page to the set
formAddPageElem.onsubmit = function(e) {
    e.preventDefault();
    
    let cardInfo = {
        url: inputPageUrl.value,
        title: inputPageTitle.value,
        thumbnail: ''
    };
    
    if (cardInfo.url == '')
        return;
    
    for (let page of pageList) {
        if (page.url === cardInfo.url)
            return;
    }
    
    window.viper.favoritePageManager.addFavorite(cardInfo.url, cardInfo.title);
    
    let cardId = pageList.unshift(cardInfo);
    pageList.pop();
    
    // Update the DOM
    let cardHtml = cellTemplateNoThumbnail.replace(/{{id}}/g, cardId)
                                          .replace(/{{url}}/g, cardInfo.url)
                                          .replace(/{{title}}/g, cardInfo.title)
                                          .replace(/{{imgSrc}}/g, cardInfo.thumbnail);
    
    let mainContainer = document.getElementById('mainGrid');
    mainContainer.removeChild(mainContainer.lastChild);
    
    let currentHtml = mainContainer.innerHTML;
    mainContainer.innerHTML = cardHtml + currentHtml;
    
    addPageDialog.style.display = 'none';
};

// Allow for user to modify the cards on the page
document.addEventListener('click', function(e) {
    if (!e.target || pageList.length === 0)
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
    let p = target.parentNode.parentNode;
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
