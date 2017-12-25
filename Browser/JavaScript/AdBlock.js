(function() {
/// Handles the :has(...) cosmetic filter option
function hideIfHas(subject, target) {
    var needScope = /^\s*[+>~]/;
    if (needScope.test(target)) {
        target = ':scope ' + target;
    }

    var nodes = document.querySelectorAll(subject), i;
    for (i = 0; i < nodes.length; ++i) {
        if (nodes[i].querySelector(target) !== null) {
            nodes[i].style.display = 'none';
        }
    }
}

/// Handles the :xpath(...) cosmetic filter option
function doXPath(expr, cssSelector) {
    var output = [], i, j;
    var xpathExpr = document.createExpression(expr, null);
    var xpathResult = null;
    var nodes = document.querySelectorAll(cssSelector);
    for (i = 0; i < nodes.length; ++i) {
        xpathResult = xpathExpr.evaluate(nodes[i], XPathResult.UNORDERED_NODE_SNAPSHOT_TYPE, xpathResult);
        j = xpathResult.snapshotLength;
        while (j--) {
            xpathResult.snapshotItem(j).style.display = 'none';
        }
    }
}

{{ADBLOCK_INTERNAL}}

})();
