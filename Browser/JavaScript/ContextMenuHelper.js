(function() {
    function qualifyURL(url) {
        var a = document.createElement('a');
        a.href = url;
        return a.href;
    }
    var e = document.elementFromPoint((window.pageXOffset + {{x}}), (window.pageYOffset + {{y}}));
    var imgUrl = '';
    if (e && e.tagName.toLowerCase() == 'img') { imgUrl = qualifyURL(e.src); }
    return imgUrl;
})();
