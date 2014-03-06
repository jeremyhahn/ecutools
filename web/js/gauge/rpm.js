var guage.rpm = {};

guage.rpm.insertAt = function(div) {
    var data = google.visualization.arrayToDataTable([
      ['Label', 'Value'],
      ['RPM', 0]
    ]);
	var rpmGuage = new google.visualization.Gauge(document.getElementById(div));
    rpmGuage.draw(data, getOptions());
};

guage.rpm.getOptions = function() {
	return {
      width: 400, height: 120,
      redFrom: 90, redTo: 100,
      yellowFrom:75, yellowTo: 90,
      minorTicks: 5,
      min: 0,
      max: 8000,
      height: 200,
      width: 200
    };
};

guage.rpm.update = function() {
	rpmGuage.draw(getRpmData(), getOptions());
 	setTimeout(update, 500);
};

guage.rpm.simulate = function() {
    return google.visualization.arrayToDataTable([
		  ['Label', 'Value'],
		  ['RPM', Math.floor((Math.random()*8000)+1)]
	]);
    update();
};