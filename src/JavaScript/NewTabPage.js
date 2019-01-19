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
        
const cellTemplate = '<div class="cell"><a href="{{url}}"><img class="thumbnail" src="{{imgSrc}}" alt="{{title}}">'
            + '<div class="titleContainer"><div class="titleTextWrapper"><span class="title">{{title}}</span></div></div></a></div>';

getFavoritePages().then(function(result) {
    if (!result)
        return;
            
    var mainContainer = document.getElementById('mainGrid');
    for (var i = 0; i < result.length; ++i) {
        var item = result[i];
        if (item == null || !('url' in item) || !('title' in item) || !('thumbnail' in item))
            continue;
        var itemHtml = cellTemplate.replace(/{{url}}/g, item.url)
                                   .replace(/{{imgSrc}}/g, item.thumbnail)
                                   .replace(/{{title}}/g, item.title);
        mainContainer.innerHTML += itemHtml;
    }
});
