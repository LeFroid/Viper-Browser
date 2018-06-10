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
    var e = document.elementFromPoint(%1, %2);
    var result = {
        linkUrl: '',
        mediaUrl: '',
        mediaType: 0,
        selectedText: '',
        isEditable: false,
        coords: [(%1 + window.pageXOffset), (%2 + window.pageYOffset)]
    };
    while (e) {
        if (!result.isEditable) { result.isEditable = isElemEditable(e); }
        if (result.selectedText == '' && isSelected(e)) { result.selectedText = window.getSelection().toString(); }
        if (result.linkUrl == '') {
            if (e.tagName.toLowerCase() == 'a') {
                result.linkUrl = e.getAttribute('href');
            } else {
                var aNearE = e.closest('a[href]');
                if (aNearE) { result.linkUrl = aNearE.href; }
            }
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
        e = e.parentElement;
    }
    return result;
})();
