var express = require('express');
var panel = require('./build/Release/panel.node');


var app = express();
var configured = false;

app.get('/:command', function (req, res) {

	console.log(req.params.command);


	if(req.params.command == 'setup'){
		configured = panel.setup();
		console.log(configured);
		
	}else if(req.params.command == 'run'){
		if (configured)
			panel.run();

		res.send("OK!");

	}else if(req.params.command == 'exit'){
		if(configured){
			panel.exit('0');
			configured = false;
		}
		res.send("OK!");

	}else if(req.params.command == 'on'){
		
			console.log(panel.digital(1));
			res.send("OK!");
	   

	}else if(req.params.command == 'off'){
			console.log(panel.digital(0));
			res.send("OK!");

	}else if(req.params.command == 'get'){
		    var readings= panel.getvalues();
			console.log(readings);
			res.send(readings);

	}else if(req.params.command == 'getall'){
			console.log(panel.getallvalues());
			res.send("OK!");

	}			

});





var server = app.listen(80, function () {

  var host = server.address().address;
  var port = server.address().port;
  
  console.log('App listening at http://%s:%s', host, port);
  configured = panel.setup();
  panel.run();


});

