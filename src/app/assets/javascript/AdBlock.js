(function() {
/// Checks if the given string needs ':scope' prepended to it
const addScopeIfNeeded = function(str) {
    var needScope = /^\s*[+>~]/;
    if (needScope.test(str)) {
        str = ':scope ' + str;
    }
    return str;
};
const hideIfHasAsync = async function(subject, target, root) {
    var nodes = root.querySelectorAll(subject);
    var i = 0;
    for (; i < nodes.length; ++i) {
        if (nodes[i++].nodeName === 'BODY')
            break;
    }
    for (; i < nodes.length; ++i) {
        let currNode = nodes[i];
        let compStyle = window.getComputedStyle(currNode);
        if (compStyle.cssText && compStyle.cssText.indexOf(target) >= 0) {
            currNode.style.cssText = 'display: none !important;';
        }
    }
};
/// Handles the :has(...) cosmetic filter option
const hideIfHas = function (subject, target, root) {
    target = addScopeIfNeeded(target);

    if (root === undefined)
        root = document;

    if (subject === '*') {
        (async(sub, tar, rt) => {
            hideIfHasAsync(sub, tar, rt);
        })(subject, target, root);
        return;
    }
    var nodes = root.querySelectorAll(subject), i;
    for (i = 0; i < nodes.length; ++i) {
        if (nodes[i].querySelector(target) !== null) {
            nodes[i].style.cssText = 'display: none !important;';
        }
    }
};
const hideIfNotHasAsync = async function(subject, target, root) {
    var nodes = root.querySelectorAll(subject);
    var i = 0;
    for (; i < nodes.length; ++i) {
        if (nodes[i++].nodeName === 'BODY')
            break;
    }
    for (; i < nodes.length; ++i) {
        let currNode = nodes[i];
        let compStyle = window.getComputedStyle(currNode);
        if (compStyle.cssText && compStyle.cssText.indexOf(target) >= 0) {
            currNode.style.cssText = 'display: none !important;';
        }
    }
};
/// Handles :if-not(...) cosmetic filter option if it does not have any nested cosmetic filter options
const hideIfNotHas = function (subject, target, root) {
    target = addScopeIfNeeded(target);

    if (root === undefined)
        root = document;

    if (subject === '*') {
        (async(sub, tar, rt) => {
            hideIfNotHasAsync(sub, tar, rt);
        })(subject, target, root);
        return;
    }
    var nodes = root.querySelectorAll(subject), i;
    for (i = 0; i < nodes.length; ++i) {
        if (nodes[i].querySelector(target) === null) {
            nodes[i].style.cssText = 'display: none !important;';
        }
    }
};
/// Handles the :has-text(...) cosmetic filter option, returns an array of elements that match the given criteria
const hasText = function (selector, text, root) {
    if (root === undefined) {
        root = document;
    }
    var elements = selector === '' ? [ root ] : root.querySelectorAll(selector);
    var subElems = [].filter.call(elements, function(element) {
        return RegExp(text).test(element.textContent);
    });
    return subElems;
};
const minTextLength = function (selector, minLength, root) {
    if (root === undefined)
        root = document;

    if (typeof(minLength) == 'string')
        minLength = parseInt(minLength);
    
    if (isNaN(minLength) || minLength < 0)
        return [];

    let output = [];
    let elements = selector === '' ? [ root ] : root.querySelectorAll(selector);
    for (let i = 0; i < elements.length; ++i) {
        let elem = elements[i];
        if (elem.textContent.length >= minLength)
            output.push(elem);
    }
    return output;
};
const matchesCSS = function (selector, text, root, pseudoSelector) {
    if (root === undefined)
        root = document;

    if (pseudoSelector === undefined)
        pseudoSelector = null;

    var output = [];
    var nodes, i, colonIdx, attrName, attrVal;
    nodes = selector === '' ? [ root ] : root.querySelectorAll(selector);

    if (!nodes || !nodes.length)
        return output;

    colonIdx = text.indexOf(':');
    if (colonIdx < 0) { return output; }
    attrName = text.slice(0, colonIdx).trim();
    attrVal = text.slice(colonIdx + 1).trim();

    for (i = 0; i < nodes.length; ++i) {
        var compStyle = window.getComputedStyle(nodes[i], pseudoSelector);
        if (RegExp(attrVal).test(compStyle[attrName])) {
            output.push(nodes[i]);
        }
    }
    return output;
};
const matchesCSSBefore = function(selector, text, root) {
    return matchesCSS(selector, text, root, ':before');
};
const matchesCSSAfter = function(selector, text, root) {
    return matchesCSS(selector, text, root, ':after');
};
/// Handles the :xpath(...) cosmetic filter option, returns an array of elements that match the given criteria
const doXPath = function (subject, expr, root) {
    if (root === undefined) {
        root = document;
    }
    var output = [], i, j;
    var xpathExpr = document.createExpression(expr, null);
    var xpathResult = null;
    var nodes = root.querySelectorAll(subject), node = null;
    for (i = 0; i < nodes.length; ++i) {
        xpathResult = xpathExpr.evaluate(nodes[i], XPathResult.UNORDERED_NODE_SNAPSHOT_TYPE, xpathResult);
        j = xpathResult.snapshotLength;
        while (j--) {
            node = xpathResult.snapshotItem(j);
            if (node.nodeType === 1) {
                output.push(node);
            }
        }
    }
    return output;
};
/// Similar to the doXPath function, except this traverses the dom tree without the xpath expression evaluations
const nthAncestor = function (subject, expr, root) {
    if (root === undefined)
        root = document;

    var output = [], i, j;
    var nodes = root.querySelectorAll(subject), node = null;

    //expr = nth parent
    for (i = 0; i < nodes.length; ++i) {
        node = nodes[i];
        for (j = 0; j < expr; ++j) {
            node = node.parentElement;
            if (!node)
                break;
        }
        if (node)
            output.push(node);
    }
    return output;
};

/// Successor to the :nth-ancestor directive. Accepts an integer or CSS selector as an argument, and looks for the matching ancestor (or nearest match)
const upwardMatch = function (subject, expr, root) {
    if (root === undefined)
        root = document;

    let output = [], i, j;
    let nodes = root.querySelectorAll(subject), node = null;

    const exprAsNum = parseInt(expr, 10);
    const isExprNum = !isNaN(exprAsNum);

    for (i = 0; i < nodes.length; ++i) {
        node = nodes[i];

        if (isExprNum) {
            for (j = 0; j < exprAsNum; ++j) {
                node = node.parentElement;
                if (!node) {
                    break;
                }
            }
        } else {
            const nodeParent = node.parentElement;
            if (!nodeParent) {
                continue;
            }
            node = nodeParent.closest(expr);
        }

        if (node) {
            output.push(node);
        }
    }
    return output;
};

/// Simply removes any matching nodes from the DOM. This will always return an empty list, even when a match is removed
const removeNodes = function (subject, expr, root) {
    if (root === undefined) {
        root = document;
    }

    const nodes = root.querySelectorAll(subject);
    for (const node of nodes) {
        node.remove();
    }

    return [];
}

/// Hides each subject in the document, for which the result of the callback with parameters chainSubject, chainTarget is not null or empty
function hideIfChain(subject, chainSubject, chainTarget, callback) {
    chainSubject = addScopeIfNeeded(chainSubject);

    var nodes = document.querySelectorAll(subject), i;
    for (i = 0; i < nodes.length; ++i) {
        var result = callback(chainSubject, chainTarget, nodes[i]);
        if (result && result.length) {
            nodes[i].style.cssText = 'display: none !important;';
        }
    }
}
/// Hides each subject in the document, for which the result of the callback with parameters chainSubject, chainTarget is null or empty
function hideIfNotChain(subject, chainSubject, chainTarget, callback) {
    chainSubject = addScopeIfNeeded(chainSubject);

    var nodes = document.querySelectorAll(subject), i;
    for (i = 0; i < nodes.length; ++i) {
        var result = callback(chainSubject, chainTarget, nodes[i]);
        if (!result || (result.length < 1)) {
            nodes[i].style.cssText = 'display: none !important;';
        }
    }
}
/// Hides the nodes in the given array
function hideNodes(cb, cbSubj, cbTarget) {
    var nodes;
    if (cbSubj !== undefined) {
        nodes = cb(cbSubj, cbTarget);
    } else {
        nodes = cb;
    }
    if (nodes === null) { return; }
    var i;
    for (i = 0; i < nodes.length; ++i) {
        if (nodes[i] !== null) {
            nodes[i].style.cssText = 'display: none !important;';
        }
    }
}

{{ADBLOCK_INTERNAL_SCRIPTLET}}

const observerLimit = 7;
let mutCount = 0;
let docObserver = {};
let currentLocation = window.location ? window.location.pathname : '/';
const mutOptions = { childList: true, subtree: true };
const mutCallback = () => {

  if (mutCount++ >= observerLimit) {
    docObserver.disconnect();
    return;
  }

setTimeout(() => {

{{ADBLOCK_INTERNAL_COSMETIC}}

}, 50);

};

mutCallback();

docObserver = new MutationObserver(mutCallback);

if (document.body) {
  docObserver.observe(document.body, mutOptions);
} else {
  document.addEventListener('readystatechange', (e) => {
    if (document.readyState == 'interactive') {
      docObserver.observe(document.body, mutOptions);
    }
  });
}

const urlEventTypes = [ 'click', 'popstate', 'onload' ];
urlEventTypes.forEach((eventName) => {
  window.addEventListener(eventName, (e) => {
    if (window.location.pathname != currentLocation) {
      mutCount = 0;
      currentLocation = window.location.pathname;
      setTimeout(() => { mutCallback(); }, 500);
    }
  });
});

})();
