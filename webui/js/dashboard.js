var dashboard = {};
dashboard.gauge = {};
dashboard.websocket = null;
dashboard.isTuning = false;

dashboard.streamGaugeData = function() {

	//dashboard.websocket.send('stream_gauge_data');
	//if(dashboard.isTuning)
	   //setTimeout('dashboard.streamGaugeData()', 250);
	//	setTimeout('dashboard.streamGaugeData()', 250);
}

dashboard.load = function() {

	google.load("visualization", "1", {packages:["gauge"]});
    google.setOnLoadCallback(function() {

    	dashboard.rpm.load();
    	dashboard.ignitionTotalTiming.load();
    	dashboard.manifoldRelativePressure.load();
    	dashboard.boostError.load();
    	dashboard.primaryWastegateDuty.load();
    	dashboard.throttleOpeningAngle.load();
    	dashboard.coolFueling.load();
    	dashboard.afCorrection1.load();
    	dashboard.afLearning1.load();
    	dashboard.massAirflow.load();
    	dashboard.engineLoadDirect.load();
    	dashboard.feedbackKnockCorrection.load();
    	dashboard.fineLearningKnockCorrection.load();
    	dashboard.ignitionAdvanceMultiplier.load();

    	if(dashboard.websocket == null) {
           dashboard.websocket = new WebSocket("ws://ecutools.io:8080/ecutune/dashboard");
        }

        dashboard.websocket.onopen = function() {
            console.log("Socket opened!");
            dashboard.isTuning = true;
            dashboard.streamGaugeData();
        }

        dashboard.websocket.onmessage = function(message) {

        	//console.log('Receiving message');
        	//console.debug(message);

        	var gaugeValues = message.data.split(',');
        	for(i=0; i<gaugeValues.length; i++) {
        		var gaugePieces = gaugeValues[i].split(' ');
        		var gauge = gaugePieces[0];
        		var value = gaugePieces[1];

        		//console.log("Gauage: " + gauge);
        		//console.log("Value: " + value);

        		if(!dashboard.isTuning) {
        			dashboard.rpm.gauge.setValue(0);
        			dashboard.ignitionTotalTiming.gauge.setValue(0);
        			dashboard.manifoldRelativePressure.gauge.setValue(0);
        			dashboard.boostError.gauge.setValue(0);
        			dashboard.primaryWastegateDuty.gauge.setValue(0);
        			dashboard.throttleOpeningAngle.gauge.setValue(0);
        			dashboard.coolFueling.gauge.setValue(0);
        			dashboard.afCorrection1.gauge.setValue(0);
        			dashboard.afLearning1.gauge.setValue(0);
        			dashboard.massAirflow.gauge.setValue(0);
        			dashboard.engineLoadDirect.gauge.setValue(0);
        			dashboard.feedbackKnockCorrection.gauge.setValue(0);
        			dashboard.fineLearningKnockCorrection.gauge.setValue(0);
        			dashboard.ignitionAdvanceMultiplier.gauge.setValue(0);
        			return;
        		}

        		if(gauge == 'rpm') {
        			dashboard.rpm.gauge.setValue(parseInt(value));
        		}
        		else if(gauge == 'itt') {
        			dashboard.ignitionTotalTiming.gauge.setValue(parseInt(value));
        		}
        		else if(gauge == 'mrp') {
        			dashboard.manifoldRelativePressure.gauge.setValue(parseInt(value));
        		}
        		else if(gauge == 'be') {
        			dashboard.boostError.gauge.setValue(parseInt(value));
        		}
        		else if(gauge == 'pwd') {
        			dashboard.primaryWastegateDuty.gauge.setValue(parseInt(value));
        		}
        		else if(gauge == 'toa') {
        			dashboard.throttleOpeningAngle.gauge.setValue(parseInt(value));
        		}
        		else if(gauge == 'cf') {
        			dashboard.coolFueling.gauge.setValue(parseInt(value));
        		}
        		else if(gauge == 'afc1') {
        			dashboard.afCorrection1.gauge.setValue(parseInt(value));
        		}
        		else if(gauge == 'afl1') {
        			dashboard.afLearning1.gauge.setValue(parseInt(value));
        		}
        		else if(gauge == 'ma') {
        			dashboard.massAirflow.gauge.setValue(parseInt(value));
        		}
        		else if(gauge == 'eld') {
        			dashboard.engineLoadDirect.gauge.setValue(parseInt(value));
        		}
        		else if(gauge == 'fkc') {
        			dashboard.feedbackKnockCorrection.gauge.setValue(parseInt(value));
        		}
        		else if(gauge == 'flkc') {
        			dashboard.fineLearningKnockCorrection.gauge.setValue(parseInt(value));
        		}
        		else if(gauge == 'iam') {
        			dashboard.ignitionAdvanceMultiplier.gauge.setValue(parseInt(value));
        		}
        		else {
        			console.error('Discarding unknown gauge metric: ' + gauge);
        		}

        		if(value == 0) dashboard.isTuning = false;
        	}
        }

        dashboard.websocket.onclose = function() {
        	dashboard.isTuning = false;
        }
    });
};

// RPM / Engine Load
dashboard.rpm = {};
dashboard.rpm.gauge = {};
dashboard.rpm.load = function() {
	dashboard.rpm.gauge = new Gauge('RPM', {
	    redFrom: 7000,
	    redTo: 8000,
	    yellowFrom: 6000, 
	    yellowTo: 7000,
	    minorTicks: 5,
	    min: 0,
	    max: 8000,
	    height: 200,
	    width: 200
	});
	dashboard.rpm.gauge.drawAt('gauge_rpm');
};

// Ignition Total Timing
dashboard.ignitionTotalTiming = {};
dashboard.ignitionTotalTiming.load = function() {
	dashboard.ignitionTotalTiming.gauge = new Gauge('ITT', {
        width: 400, height: 120,
        minorTicks: 5,
        min: 0,
        max: 100,
        height: 200,
        width: 200
      });
	dashboard.ignitionTotalTiming.gauge.drawAt('gauge_ignition_total_timing');
};

// Manifold Relative Pressure
dashboard.manifoldRelativePressure = {};
dashboard.manifoldRelativePressure.load = function() {

	dashboard.manifoldRelativePressure.gauge = new Gauge('MRP', {
        width: 400, height: 120,
        minorTicks: 5,
        min: 0,
        max: 100,
        height: 200,
        width: 200
      });
	dashboard.manifoldRelativePressure.gauge.drawAt('gauge_manifold_pressure');
};

// Boost Error
dashboard.boostError = {};
dashboard.boostError.load = function() {

	dashboard.boostError.gauge = new Gauge('BE', {
        width: 400, height: 120,
        minorTicks: 5,
        min: 0,
        max: 100,
        height: 200,
        width: 200
      });
	dashboard.boostError.gauge.drawAt('gauge_boost_error');
};

// Primary Wastegate Duty
dashboard.primaryWastegateDuty = {};
dashboard.primaryWastegateDuty.load = function() {

	dashboard.primaryWastegateDuty.gauge = new Gauge('PWD', {
        width: 400, height: 120,
        minorTicks: 5,
        min: 0,
        max: 100,
        height: 200,
        width: 200
      });
	dashboard.primaryWastegateDuty.gauge.drawAt('gauge_primary_wastegate_duty');
};

// Throttle Opening Angle
dashboard.throttleOpeningAngle = {};
dashboard.throttleOpeningAngle.load = function() {

	dashboard.throttleOpeningAngle.gauge = new Gauge('TOA', {
        width: 400, height: 120,
        minorTicks: 5,
        min: 0,
        max: 100,
        height: 200,
        width: 200
      });
	dashboard.throttleOpeningAngle.gauge.drawAt('gauge_throttle_opening_angle');
};

// CO/OL Fueling
dashboard.coolFueling = {};
dashboard.coolFueling.load = function() {

	dashboard.coolFueling.gauge = new Gauge('CO/OL', {
        width: 400, height: 120,
        minorTicks: 5,
        min: 0,
        max: 100,
        height: 200,
        width: 200
      });
	dashboard.coolFueling.gauge.drawAt('gauge_co_ol_fueling');
};

// A/F Correction #1
dashboard.afCorrection1 = {};
dashboard.afCorrection1.load = function() {

	dashboard.afCorrection1.gauge = new Gauge('AFC 1', {
        width: 400, height: 120,
        minorTicks: 5,
        min: 0,
        max: 100,
        height: 200,
        width: 200
      });
	dashboard.afCorrection1.gauge.drawAt('gauge_af_correction');
};

// A/F Learning #1
dashboard.afLearning1 = {};
dashboard.afLearning1.load = function() {

	dashboard.afLearning1.gauge = new Gauge('AFL 1', {
        width: 400, height: 120,
        minorTicks: 5,
        min: 0,
        max: 100,
        height: 200,
        width: 200
      });
	dashboard.afLearning1.gauge.drawAt('gauge_af_learning');
};

// Mass Airflow
dashboard.massAirflow = {};
dashboard.massAirflow.load = function() {

	dashboard.massAirflow.gauge = new Gauge('MAF', {
        width: 400, height: 120,
        minorTicks: 5,
        min: 0,
        max: 100,
        height: 200,
        width: 200
      });
	dashboard.massAirflow.gauge.drawAt('gauge_mass_airflow');
};

// Engine Load (Direct)
dashboard.engineLoadDirect = {};
dashboard.engineLoadDirect.load = function() {

	dashboard.engineLoadDirect.gauge = new Gauge('ELD', {
        width: 400, height: 120,
        minorTicks: 5,
        min: 0,
        max: 100,
        height: 200,
        width: 200
      });
	dashboard.engineLoadDirect.gauge.drawAt('gauge_engine_load');
};

// Feedback Knock Correction
dashboard.feedbackKnockCorrection = {};
dashboard.feedbackKnockCorrection.load = function() {

	dashboard.feedbackKnockCorrection.gauge = new Gauge('FKC', {
        width: 400, height: 120,
        minorTicks: 5,
        min: 0,
        max: 100,
        height: 200,
        width: 200
      });
	dashboard.feedbackKnockCorrection.gauge.drawAt('gauge_feedback_knock_correction');
};

// Fine Learning Knock Correction
dashboard.fineLearningKnockCorrection = {};
dashboard.fineLearningKnockCorrection.load = function() {

	dashboard.fineLearningKnockCorrection.gauge = new Gauge('FLKC', {
        width: 400, height: 120,
        minorTicks: 5,
        min: 0,
        max: 100,
        height: 200,
        width: 200
      });
	dashboard.fineLearningKnockCorrection.gauge.drawAt('gauge_fine_learning_knock_correction');
};

// IAM (Ignition Advance Multiplier)
dashboard.ignitionAdvanceMultiplier = {};
dashboard.ignitionAdvanceMultiplier.load = function() {

	dashboard.ignitionAdvanceMultiplier.gauge = new Gauge('IAM', {
        width: 400, height: 120,
        minorTicks: 5,
        min: 0,
        max: 100,
        height: 200,
        width: 200
      });
	dashboard.ignitionAdvanceMultiplier.gauge.drawAt('gauge_ignition_advance_multiplier');
};