(function() {
    function qualifyURL(url) {
        var a = document.createElement('a');
        a.href = url;
        return a.href;
    }
    function isSelected(e) {
        var s = window.getSelection();
        if (s.type != 'Range')
            return false;
        return s.containsNode(e, true);
    }
    function isElemEditable(e) {
        if (e.isContentEditable) { return true; }
        if (e.tagName.toLowerCase() == 'input' || e.tagName.toLowerCase() == 'textarea')
            return e.getAttribute('readonly') != 'readonly';
		return false;
    }
    function getElementsFromPoint(x, y) {
        var elem, elements = [];
        var oldVis = [];
        while (true) {
            elem = document.elementFromPoint(x, y);
            if (!elem || elem === document.documentElement) {
                break;
            }
            elements.push(elem);
            oldVis.push(elem.style.visibility);
            elem.style.visibility = 'hidden';
        }
        for (var k = 0; k < elements.length; k++) { elements[k].style.visibility = oldVis[k]; }
        elements.reverse();
        return elements;
    }
    var elems = getElementsFromPoint(%1, %2);
    if (elems.length < 1) { elems.push(document.activeElement); }
    var result = {
        linkUrl: '',
        mediaUrl: '',
        mediaType: 0,
        selectedText: '',
        isEditable: false,
        coords: [%1, %2]
    };
    for (var i = 0; i < elems.length; ++i) {
        var e = elems[i];
        if (!result.isEditable) { result.isEditable = isElemEditable(e); }
        if (result.selectedText == '' && isSelected(e)) { result.selectedText = window.getSelection().toString(); }
        if (result.linkUrl == '') {
            var aNearE = e.closest('a[href]');
            if (aNearE) { result.linkUrl = aNearE.href; }
        }
        if (result.mediaUrl == '') {
            if (e.tagName.toLowerCase() == 'img') { result.mediaType = 1; result.mediaUrl = qualifyURL(e.src); }
            if (e.tagName.toLowerCase() == 'video') {
                result.mediaType = 2;
                result.mediaUrl = qualifyURL(e.currentSrc);
            }
            if (e.tagName.toLowerCase() == 'audio') {
                result.mediaType = 3;
                result.mediaUrl = qualifyURL(e.currentSrc);
            }
        }
    }
    return result;
})();
