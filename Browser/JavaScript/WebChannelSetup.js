(function() {
    %1

    new QWebChannel(qt.webChannelTransport, function(channel) {
        window.viper = {};
        window.viper.storage = channel.objects.extStorage;
        var event = document.createEvent('Event');
        event.initEvent('_webchannel_setup', true, true);
        window._webchannel_initialized = true;
        document.dispatchEvent(event);
    });
})();
