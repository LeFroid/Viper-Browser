(function() {
    var autoFillVals = {};
    %1

    var fillInFor = function(doc) {
        var inputElems = doc.getElementsByTagName('input');
        for (let e of inputElems) {
            let eType = e.type.toLowerCase();
            let eMode = ('inputmode' in e) ? e.inputmode.toLowerCase() : eType;
            if (eType == 'email' || eMode == 'email' 
                    || eType == 'password' || eType == 'text' || eMode == 'text') {
                if (e.name in autoFillVals) {
                    e.value = autoFillVals[e.name];
                }
            }
        }
    };

    fillInFor(document);
    var subFrames = document.getElementsByTagName('iframe');
    if (subFrames && subFrames.length > 0) {
        for (var i = 0; i < subFrames.length; ++i) {
            try {
                fillInFor(subFrames[i].contentDocument);
            } catch(ex) {}
        }
    }
})();
