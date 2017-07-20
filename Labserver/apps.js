// Setup basic express server
var panel = require('./build/Release/panel.node');
var express = require('express');
var app = express();

var server = require('http').createServer(app);
var io = require('socket.io')(server);
var port = 80;

app.use(express.static(__dirname + '/public'));

server.listen(port, function () {
    console.log('Server listening at port %d', port);

});


var configured = false, authenticated = false;

io.on('connection', function (socket) {
    var password = "";
    var duration = 0, interval = 0;
    var step = 2;


    function SendMessage(socket) {

        if (duration > 0) {
            var data = panel.getvalues();
            console.log(data);
            socket.emit('data received', data);
            duration--;
        } else {
            clearInterval(interval);
            panel.digital(0);
        }

    }

    socket.on('new connection', function (data) {
        // fazer acesso ao rlms para autenticar
        //password = data.pass;
        console.log('new connection' + data);
        authenticated = true;
    });


    socket.on('start', function (data) {
        if (!configured) {
            configured = panel.setup();
            if (configured)
                console.log("thread : " + panel.run());
            else
                console.log('Emit error here');
        }
        console.log(data);
        if (data.sw > 0) {
            panel.digital(1);
            clearInterval(interval);
            duration = data.duration;
            step = (data.step > 1) ? data.step : 2;
            interval = setInterval(function () {
                SendMessage(socket);
            }, step * 1000);
            SendMessage(socket);

        } else {
            panel.digital(0);
            clearInterval(interval);
        }
    });


    socket.on('disconnect', function () {
        clearInterval(interval);
        console.log('disconnected');
        //panel.digital(0);
        setTimeout(function () {
            if (configured) {
                configured = authenticated = false;
                panel.exit();
            }             
        }, 1000);
    });


});
