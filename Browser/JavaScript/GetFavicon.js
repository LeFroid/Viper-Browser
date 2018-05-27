(function() {
    var lnk = document.querySelector("link[rel*='icon']");
    if (lnk !== undefined && lnk !== null)
        return lnk.getAttribute("href");
    return null;
})();
