(function() {
    var autoFillVals = {};
    %1
    var inputElems = document.getElementsByTagName('input');
    for (let e of inputElems) {
        let eType = e.type.toLowerCase();
        if (eType === 'email' || eType === 'password' || eType === 'text') {
            if (e.name in autoFillVals) {
                e.value = autoFillVals[e.name];
                e.dispatchEvent(new Event('change'));
            }
        }
    }
})();
