(function() {
    %1

    function setupWebChannel() {
        new QWebChannel(qt.webChannelTransport, function(channel) {
            const viper = {};
            viper.storage = channel.objects.extStorage;
            window.viper = viper; 
            if (!window._webchannel_initialized) {
                var event = document.createEvent('Event');
                event.initEvent('_webchannel_setup', true, true);
                window._webchannel_initialized = true;
                document.dispatchEvent(event);
            }
        });
    }

    function attemptSetup() {
        try {
            setupWebChannel();
        } catch (e) {
            setTimeout(attemptSetup, 100);    
        }
    }

    attemptSetup();
})();
