(function() {
    function qualifyURL(url) {
        var a = document.createElement('a');
        a.href = url;
        return a.href;
    }

    var lnk = document.querySelector("link[rel*='icon']");
    if (lnk !== undefined && lnk !== null) {
        var favUrl = qualifyURL(lnk.href);
        window.viper.favicons.updateIconUrl(favUrl);
    }
})();
