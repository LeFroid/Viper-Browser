(function() {
    var onWebChannelSetup = function(cb) {
        if (window._webchannel_initialized) {
            cb();
        } else {
            document.addEventListener("_webchannel_setup", cb);
        }
    };

    function qualifyURL(url) {
        var a = document.createElement('a');
        a.href = url;
        return a.href;
    }

    var lnk = document.querySelector("link[rel*='icon']");
    if (lnk !== undefined && lnk !== null) {
        var favUrl = qualifyURL(lnk.href);
        onWebChannelSetup(() => {
            window.viper.favicons.updateIconUrl(favUrl);
        });
    }
})();
