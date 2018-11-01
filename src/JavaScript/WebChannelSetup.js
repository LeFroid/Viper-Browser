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

    try {
        if (self !== top) {
            if (top._webchannel_initialized) {
                window.viper = top.viper;
                notifySetupComplete();
            } else {
                top.document.addEventListener('__webchannel_setup', function() {
                    window.viper = top.viper;
                    notifySetupComplete();
                });
            }
        } else {
            attemptSetup();
        }
    } catch (e){}
})();
