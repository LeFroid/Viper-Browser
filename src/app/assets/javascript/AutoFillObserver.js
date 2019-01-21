(function() {

    var onSubmitForm = function(e) {
        var targetElem = e.target;
        if (targetElem.nodeName.toLowerCase() != 'form') { 
            targetElem = targetElem.parentElement;
        }

        var username = '', password = '';
        var formData = {};
        while (targetElem != undefined) {
            if (username.length > 0 && password.length > 0) {
                break;
            }

            var results = targetElem.querySelectorAll('input');
            if (results) {
                for (var i = 0; i < results.length; ++i) {
                    var elem = results[i];
                    let eType = elem.type.toLowerCase();
                    let eMode = ('inputmode' in elem) ? elem.inputmode.toLowerCase() : eType;
                    if (eType == 'email' || eType == 'password' || eType == 'text') {
                        let eName = (elem.name !== undefined) ? elem.name : eMode;
                        formData[eName] = elem.value;
                
                        if ((eType == 'text' || eMode == 'text') 
                                && (eName.indexOf('user') >= 0 || eName.indexOf('name') >= 0 || eName.indexOf('login') >= 0)) {
                            username = elem.value;
                        } else if ((eType == 'email' || eMode == 'email') && username == '') {
                            username = elem.value;
                        } else if (eType == 'password') {
                            password = elem.value;
                        } else if (elem.value.indexOf('@') >= 0 && username == '') {
                            username = elem.value;
                        }
                    }
                }
            }
            targetElem = targetElem.parentElement;
        }

        if (password.length > 0) {
            try {
                viper.autofill.onFormSubmitted(window.location.href, username, password, formData);
            } catch (ex) {
                window.parent.postMessage({
                    '_viper_webchannel_msg':  true,
                    '_viper_webchannel_obj':  'autofill',
                    '_viper_webchannel_fn':   'onFormSubmitted',
                    '_viper_webchannel_args': [ window.location.href, username, password, formData ]
                }, '*');
            }
        }
    };

    var addListenerToElemParent = function(child) {
        var elem = child;
        while (elem.parentElement != undefined) {
            elem = elem.parentElement;
            if (elem.nodeName.toLowerCase() == 'form') {
                elem.addEventListener('submit', onSubmitForm);
                return true;
            }
        }
        return false;
    };

    var isElemSubmit = function(elem) {
        var elemName = elem.nodeName.toLowerCase();
        if (elemName == 'form') {
            elem.addEventListener('submit', onSubmitForm);
            return true;
        } else if (elemName == 'input') {
            if (elem.type.toLowerCase() == 'submit') {
                if (!addListenerToElemParent(elem)) {
                    elem.addEventListener('submit', onSubmitForm);
                }
                return true;
            }
        } else if (elemName == 'button') {
            if (addListenerToElemParent(elem)) {
                return true;
            } else if ((elem.onsubmit != null && elem.onsubmit != undefined)
                    || (elem.onclick != null && elem.onclick != undefined)) {
                elem.addEventListener('submit', onSubmitForm);
                elem.addEventListener('click', onSubmitForm);
                return true;
            }
            var eventNames = [ 'onsubmit', 'onclick' ];
            var attrs = elem.attributes;
            for (let attr of attrs) {
                if (eventNames.indexOf(attr.name) > 0) {
                    if (!addListenerToElemParent(elem)) {
                        elem.addEventListener(attr.name.substr(2), onSubmitForm);
                    }
                    return true;
                }
            }
        }

        return false;
    };

    var childrenHaveForm = function(children) {
        for (var i = 0; i < children.length; ++i) {
            var child = children[i];
            if (isElemSubmit(child)) {
                return true;
            }

            if (child.children && childrenHaveForm(child.children)) {
                return true;
            }
        }
        return false;
    };

    var siblingsHaveForm = function(sibling) {
        while (sibling) {
            if (isElemSubmit(sibling)) {
                return true;
            } else {
                var children = sibling.children;
                if (children && childrenHaveForm(children)) {
                    return true;
                }
                sibling = sibling.nextElementSibling;
            }
        }
        return false;
    };

    var addFormListeners = function(n) {
        if (!n) {
            n = document;
        } else if (isElemSubmit(n)) {
            return;
        }

        if (n != document && !(n instanceof Element)) {
            return;
        }

        var inputs = n.querySelectorAll('input[type="password"]');
        for (var i = 0; i < inputs.length; ++i) {
            var passwordElem = inputs[i];

            var sibling = passwordElem.nextElementSibling;
            if (siblingsHaveForm(sibling)) {
                continue;
            }

            var foundSubmit = false;
            var parentElem = passwordElem.parentElement;
            while (parentElem != undefined && !foundSubmit) {
                sibling = parentElem.nextElementSibling;
                if (siblingsHaveForm(sibling)) {
                    foundSubmit = true;
                    break;
                }

                parentElem = parentElem.parentElement;
            }
        }
    };

    addFormListeners();

    var options = {
        childList: true,
        subtree: true
    };

    var mutCallback = function(mutationList, observer) {
        for (let mut of mutationList) {
            for (let node of mut.addedNodes) {
                addFormListeners(node);
            }
        }
    };

    var observer = new MutationObserver(mutCallback);
    observer.observe(document.documentElement, options);

})();
