
(function() {

    const _uuid = '{{SCRIPT_UUID}}';
    
    const GM_info = {
        'script': {{SCRIPT_OBJECT}},
        'scriptMetaStr': '{{SCRIPT_META_STR}}',
        'scriptHandler': 'GreaseMonkeyViper',
        'version': '0.1'
    };
    
    function GM_addStyle(css) {
        var head = document.getElementsByTagName('head')[0];
        if (!head) {
            document.onreadystatechange = function () {
                if (document.readyState == 'interactive') {
                    var newStyle = document.createElement('style');
                    newStyle.type = 'text/css';
                    newStyle.innerHTML = css; //.replace(/;/g, ' !important;');
                    document.getElementsByTagName('head')[0].appendChild(newStyle);
                }
            }
            return;
        }
        var style = document.createElement('style');
        style.type = 'text/css';
        style.innerHTML = css; //.replace(/;/g, ' !important;');
        head.appendChild(style);
    }
    
    function checkKey(key, funcName) {
        if (typeof key !== "string") {
          throw new Error(`${funcName} requires the first parameter to be of type string, not '${typeof key}'`);
        }
    }
    
    function GM_setValue(key, value) {
        checkKey(key, "GM_setValue");
        if (typeof value !== "string" &&
            typeof value !== "number" &&
            typeof value !== "boolean") {
          throw new Error(`GM_setValue requires the value to be of either a string, number, or boolean type, not '${typeof value}'`);
        }
        localStorage.setItem(_uuid + key, value);
    }
    
    function GM_getValue(name, defaultValue) {
        checkKey(name, "GM_getValue");
        return localStorage.getItem(_uuid + name) || defaultValue;
    }
    
    function GM_deleteValue(key) {
        checkKey(key, "GM_deleteValue");
        localStorage.removeItem(_uuid + key);
    }
    
    function GM_listValues() {
        const keys = [];
        for (let i = 0; i < localStorage.length; i++) {
            if (localStorage.key(i).startsWith(_uuid)) {
                keys.push(localStorage.key(i).slice(_uuid.length));
            }
        }
        return keys;
    }

    function GM_openInTab(url) {
        window.open(url);
    }

    function GM_setClipboard(text) {
        function onCopy(event) {
            document.removeEventListener('copy', onCopy, true);

            event.stopImmediatePropagation();
            event.preventDefault();

            event.clipboardData.setData('text/plain', text);
        }

        document.addEventListener('copy', onCopy, true);
        document.execCommand('copy');
    }
    
    function GM_xmlhttpRequest(details) {
        details.method = details.method ? details.method.toUpperCase() : "GET";

        if (!details.url) {
            throw new Error("GM_xmlhttpRequest requires an URL.");
        }

        // build XMLHttpRequest object
        const oXhr = new XMLHttpRequest();
        // run it
        if ("onreadystatechange" in details) {
            oXhr.onreadystatechange = function() {
                details.onreadystatechange(oXhr);
            };
        }
        if ("onload" in details) {
            oXhr.onload = function() { details.onload(oXhr); };
        }
        if ("onerror" in details) {
            oXhr.onerror = function () { details.onerror(oXhr); };
        }

        oXhr.open(details.method, details.url, true);

        if ("headers" in details) {
            for (const header in details.headers) {
                oXhr.setRequestHeader(header, details.headers[header]);
            }
        }

        if ("data" in details) {
            oXhr.send(details.data);
        } else {
            oXhr.send();
        }
    }

    function GM_getResourceUrl(resourceName) {
        return '';
    }

    function GM_log(text) {
        console.log(text);
    }

    const GM = {};
    GM.info = GM_info;
    
    const entries = {
        'log': GM_log,
        'addStyle': GM_addStyle,
        'openInTab': GM_openInTab,
        'xmlHttpRequest': GM_xmlhttpRequest,
    }
    for (newKey in entries) {
        let oldKey = entries[newKey];
        if (oldKey && (typeof GM[newKey] == 'undefined')) {
            GM[newKey] = function(...args) {
                return new Promise((resolve, reject) => {
                    try { resolve(oldKey(...args)); }
                    catch (e) { reject(e); }
                });
            };
        }
    };

    var onWebChannelSetup = function(cb) {
        if (window._webchannel_initialized) {
            cb();
        } else {
            document.addEventListener("_webchannel_setup", cb);
        }
    };

    GM.getValue = function(name, defaultValue) {
        return new Promise(function(resolve) {
			onWebChannelSetup(() => {
				window.viper.storage.getItem(_uuid, name, function(result) {
					resolve(result);
				});
			});
        });
    };
    
	GM.setValue = function(name, value) {
        return new Promise(function(resolve) {
			onWebChannelSetup(() => {
				window.viper.storage.setItem(_uuid, name, value);
				resolve();
			});
        });
    };
    
	GM.deleteValue = function(name) {
        return new Promise(function(resolve) {
			onWebChannelSetup(() => {
				window.viper.storage.removeItem(_uuid, name);
				resolve();
			});
        });
    };

	GM.listValues = function() {
        return new Promise(function(resolve) {
			onWebChannelSetup(() => {
				window.viper.storage.listKeys(_uuid, resolve);
			});
        });
    };

    const unsafeWindow = window;
    
    {{USER_SCRIPT}}
    
})();
