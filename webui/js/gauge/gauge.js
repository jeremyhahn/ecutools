var Gauge = function(label, options) {
	this.guage = null;
	this.label = label;
	this.options = options;
	this.data = google.visualization.arrayToDataTable([
      ['Label', 'Value'],
      [this.label, 0]
    ]);
};

Gauge.prototype.setValue = function(value) {
	this.data.setValue(0, 1, value);
	this.update();
};

Gauge.prototype.drawAt = function(div) {
 	 this.guage = new google.visualization.Gauge(document.getElementById(div));
 	 this.guage.draw(this.data, this.options);
};

Gauge.prototype.update = function() {
	this.guage.draw(this.data, this.options);
};

Gauge.prototype.simulate = function(max) {
	if(!max) max = 100;
	this.setValue(Math.floor((Math.random()*max)+1));
	var context = this;
	setTimeout(function() {
		context.simulate(max);
	}, 300);
};