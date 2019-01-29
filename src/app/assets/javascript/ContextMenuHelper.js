(function() {
    var doc = document;
    var win = window;

    function qualifyURL(url) {
        var a = document.createElement('a');
        a.href = url;
        return a.href;
    }
    function isUrlSameOrigin(url) {
        var a = document.createElement('a');
        a.href = url;
        return a.host == window.location.host;
    }
    function isSelected(e) {
        var s = win.getSelection();
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
        //https://gist.github.com/Rooster212/4549f9ab0acb2fc72fe3
        var elements = [], previousPointerEvents = [], current, i, d;
	    while ((current = doc.elementFromPoint(x,y)) && elements.indexOf(current)===-1 && current != null) {
    		elements.push(current);
	    	previousPointerEvents.push({
                value: current.style.getPropertyValue('pointer-events'),
                priority: current.style.getPropertyPriority('pointer-events')
            });
		    current.style.setProperty('pointer-events', 'none', 'important'); 
	    }
    	for(i = previousPointerEvents.length; d=previousPointerEvents[--i]; ) {
	    	elements[i].style.setProperty('pointer-events', d.value?d.value:'', d.priority); 
	    }
	    return elements;
    }

    var elems = getElementsFromPoint(%1, %2);
    if (elems.length < 1) { elems.push(document.activeElement); }

    var result = {
        linkUrl: '',
        mediaUrl: '',
        mediaType: 0,
        selectedText: '',
        hasSelection: false,
        isEditable: false,
        coords: [%1, %2]
    };

    for (var i = 0; i < elems.length; ++i) {
        var e = elems[i];
        var tagName = e.tagName.toLowerCase();
        if (tagName.endsWith('frame')) {
            if (isUrlSameOrigin(e.src)) {
                doc = e.contentWindow.document;
                win = e.contentWindow;

                var eRect = e.getBoundingClientRect();
                elems = getElementsFromPoint(%1 - eRect.left, %2 - eRect.top);
                if (elems.length < 1) { elems.push(doc.activeElement); }
                i = -1;
            }
            continue;
        }
        if (!result.isEditable) { result.isEditable = isElemEditable(e); }
        if ((!result.hasSelection || result.selectedText == '') && isSelected(e)) {
            result.selectedText = win.getSelection().toString();
            result.hasSelection = true;
        }
        if (result.linkUrl == '') {
            var aNearE = e.closest('a[href]');
            if (aNearE) { result.linkUrl = aNearE.href; }
        }
        if (result.mediaUrl == '') {
            if (tagName == 'img') {
                result.mediaType = 1;
                result.mediaUrl = qualifyURL(e.src);
            }
            if (tagName == 'video') {
                result.mediaType = 2;
                result.mediaUrl = qualifyURL(e.currentSrc);
            }
            if (tagName == 'audio') {
                result.mediaType = 3;
                result.mediaUrl = qualifyURL(e.currentSrc);
            }
        }
    }
    return result;
})();
