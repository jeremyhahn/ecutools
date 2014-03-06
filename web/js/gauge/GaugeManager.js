var GaugeManager = {};
var gauges = [];
var gaugeNames = [];

GaugeManager.add = function(name, gauge) {
	for(i=0; i<gaugeNames.length; i++) {
		if(gaugeNames[i] == name) {
			throw('gauge ' + name + ' already exists.');
		}
		if(i == gaugeNames.length) {
			gaugeNames[i++] = name;
		}
	}
	gauges.push(gauge);
};

GaugeManager.get = function(name) {
	for(i=0; i<gaugeNames.length; i++) {
		if(gaugeNames[i] == name) {
			return gauges[i];
		}
	}
};

GaugeManager.getAt = function(index) {
	return gauges[i];
};

GaugeManager.remove = function(name) {
	for(i=0; i<gaugeNames.length; i++) {
		gaugeNames.splice(index, 1);
		gauges.splice(index, 1);
	}
};

GaugeManager.removeAt = function(index) {
	gaugeNames.splice(index, 1);
	gauges.splice(index, 1);
};

GaugeManager.removeAll = function(index) {
	for(i=0; i<gauges.length; i++) {
		gaugeNames.splice(i, 1);
		gauges.splice(i, 1);
	}
};

GaugeManager.destroyAll = function() {
	for(i=0; i<gauges.length; i++) {
		gauges[i].destroy();
		gauges.splice(index, 1);
	}
};