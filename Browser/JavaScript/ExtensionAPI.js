const __extUID = '{{EXT_UID}}';

chrome.storage.get = function(keys, callback) {
    result = chrome.storage.getResult(__extUID, keys);
    callback(result);
}

