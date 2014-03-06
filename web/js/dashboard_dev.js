var websocket = null;
var dashboard = {}
var total_messages_received = 0;

dashboard.setWebsocketState = function(state) {
	document.getElementById('websocket_state').innerHTML = '<b>' + state + '</b>';
};

dashboard.send = function(data) {
	if(websocket == null) {
	   console.error('websocket closed!');
	   return false;
	}
	websocket.send(data);
}

dashboard.startLogging = function() {
	dashboard.send('cmd:log');
}

dashboard.stopLogging = function() {
	dashboard.send('cmd:nolog');
}

dashboard.applyFilter = function() {
	dashboard.send('cmd:filter:' + document.getElementById('canid_filter').value);
}

dashboard.clearFilter = function() {
	dashboard.send('cmd:nofilter');
}

dashboard.disconnect = function() {
	if(websocket == null) {
	   console.error('websocket already closed!');
	   websocket = null;
	   return false;
	}
	websocket.close();
	websocket = null;
}

dashboard.connect = function() {

	if(websocket != null) {
		console.log('websocket already connected!');
		return false;
	}

	websocket = new WebSocket("ws://localhost:8080/ecutune");

	websocket.onopen = function() {
		dashboard.setWebsocketState('OPEN');
    }

	websocket.onmessage = function(message) {

		if(message.data.substring(0, 4) == '07e8') { // sent from ecu
			console.log('%c' + message.data, 'color: green;font-weight: bold;');
		}
		else if(message.data.substring(0, 8) == '20000004') { // error
			console.log('%c' + message.data, 'color: red;font-weight: bold;');
		}
		else { // other messages on the bus
			console.log(message.data);
		}

		document.getElementById('total_messages_received').innerHTML = ++total_messages_received;
		return;

		//console.log('Receiving message');
		//console.debug(message);

		var gaugeValues = message.data.split(',');
		for(i=0; i<gaugeValues.length; i++) {
			var gaugePieces = gaugeValues[i].split(' ');
			var gauge = gaugePieces[0];
			var value = gaugePieces[1];
	
			//console.log("Gauage: " + gauge);
			//console.log("Value: " + value);
	
			switch(gauge) {
			
				case 'rpm':
					document.getElementById('rpm').innerHTML = value;
				break;
				
				case 'itt':
					document.getElementById('itt').innerHTML = value;
				break;
				
				case 'mrp':
					document.getElementById('mrp').innerHTML = value;
				break;
				
				case 'be':
					document.getElementById('be').innerHTML = value;
				break;
				
				case 'pwd':
					document.getElementById('pwd').innerHTML = value;
				break;
				
				case 'toa':
					document.getElementById('toa').innerHTML = value;
				break;
				
				case 'cf':
					document.getElementById('cf').innerHTML = value;
				break;
				
				case 'afc1':
					document.getElementById('afc1').innerHTML = value;
				break;
				
				case 'afl1':
					document.getElementById('afl1').innerHTML = value;
				break;
				
				case 'ma':
					document.getElementById('ma').innerHTML = value;
				break;
				
				case 'eld':
					document.getElementById('eld').innerHTML = value;
				break;
				
				case 'fkc':
					document.getElementById('fkc').innerHTML = value;
				break;
				
				case 'flkc':
					document.getElementById('flkc').innerHTML = value;
				break;
				
				case 'iam':
					document.getElementById('iam').innerHTML = value;
				break;
				
				default:
					console.error('Discarding unknown gauge metric: ' + gauge);
				break;
			}
		}
	}

	websocket.onclose = function() {
		dashboard.setWebsocketState('CLOSED');
	}
}

//document.addEventListener('DOMContentLoaded', function() {}, false);
