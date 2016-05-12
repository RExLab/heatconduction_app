$(function(){

var FADE_TIME = 200;
// Initialize varibles
var $window = $(window);
var $messages = $('.messages'); // Messages area
var $inputMessage = $('.inputMessage'); // Input message input box


// Prompt for setting a username
var connected = true;

var socket = io();

// Initialize varibles
var $window = $(window);
// Prompt for setting a username

socket.emit('new connection', {pass : $("#pass").html()});
		

	// Whenever the server emits 'user joined', log it in the chat body
socket.on('data received', function (data) {
	console.log(data);
});


function addMessageElement (el, options) {
	var $el = $(el);

	// Setup default options
	if (!options) {
	  options = {};
	}
	if (typeof options.fade === 'undefined') {
	  options.fade = true;
	}
	if (typeof options.prepend === 'undefined') {
	  options.prepend = false;
	}

	// Apply options
	if (options.fade) {
	  $el.hide().fadeIn(FADE_TIME);
	}
	if (options.prepend) {
	  $messages.prepend($el);
	} else {
	  $messages.append($el);
	}
	$messages[0].scrollTop = $messages[0].scrollHeight;
}


function sendMessage () {
	var message = {};
	message.duration = $("#duration").val();
	message.sw = $("#sw").val();

	// if there is a non-empty message and a socket connection
	if (message) {
	  $inputMessage.val('');
	  var time =  new Date();
	  printLog({
		date: time,
		message: message
	  });
	  message.date = time;
	  message.pass = "minhasenha334" ;
	  // tell server to execute 'new message' and send along one parameter
	  socket.emit('start', message);
	}
}

// Adds the visual chat message to the message list
function printLog (m, options) {
	// Don't fade the message in if there is an 'X was typing'
	options = options || {};

	var $dateDiv = $('<span class="date"/>').text(m.date);
	var $messageBodyDiv = $('<span class="messageBody">').text(m.message.toString());

	var $messageDiv = $('<li class="message"/>')
	  .data('date', m.date).append($dateDiv, $messageBodyDiv);

	addMessageElement($messageDiv, options);
}


// Adds a message element to the messages and scrolls to the bottom
// el - The element to add as a message
// options.fade - If the element should fade-in (default = true)
// options.prepend - If the element should prepend
//   all other messages (default = false)



// Keyboard events

$window.keydown(function (event) {
	
	// When the client hits ENTER on their keyboard
	if (event.which === 13) {
		sendMessage();
	}

});



// Click events

$('.chave').click(function () {
	var key = $(this).attr("id");
	
	if ($(this).hasClass("on")) {
		$(this).attr('class', 'chave off');

	} else if ($(this).hasClass("off")) {
		$(this).attr('class', 'chave on');
	}
	
	if (connected) {
		sendMessage();
	}
});


});