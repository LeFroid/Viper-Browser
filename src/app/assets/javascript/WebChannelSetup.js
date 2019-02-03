(function() {
    %1

    function notifySetupComplete() {
        if (!window._webchannel_initialized) {
            var event = document.createEvent('Event');
            event.initEvent('_webchannel_setup', true, true);
            window._webchannel_initialized = true;
            document.dispatchEvent(event);
        }
    }

    function setupWebChannel() {
        new QWebChannel(qt.webChannelTransport, function(channel) {
            const viper = {};
            viper.storage = channel.objects.extStorage;
            viper.favicons = channel.objects.favicons;
            viper.favoritePageManager = channel.objects.favoritePageManager;
            viper.autofill = channel.objects.autofill;
            window.viper = viper; 
            notifySetupComplete();
        });
    }

    function attemptSetup() {
        try {
            setupWebChannel();
        } catch (e) {
            setTimeout(attemptSetup, 100);    
        }
    }

    function onMessage(event) {
        try {
            var msgData = event.data;
            if (msgData._viper_webchannel_msg !== true)
                return;

            var channelObj = window.viper[msgData._viper_webchannel_obj];
            if (!channelObj || typeof(channelObj[msgData._viper_webchannel_fn]) != 'function')
                return;

            channelObj[msgData._viper_webchannel_fn].apply(channelObj, msgData._viper_webchannel_args);
        } catch(ex) {
            console.error('Caught exception while handling message to webchannel by subframe: ' + ex);
        }
    }

    try {
        if (self !== top) {
            if (top._webchannel_initialized) {
                window.viper = top.viper;
                notifySetupComplete();
            } else {
                top.document.addEventListener('_webchannel_setup', function() {
                    window.viper = top.viper;
                    notifySetupComplete();
                });
            }
        } else {
            attemptSetup();
            window.addEventListener('message', onMessage, false);
        }
    } catch (e){
    }
})();
