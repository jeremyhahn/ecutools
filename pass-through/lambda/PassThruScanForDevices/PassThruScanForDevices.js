var AWS = require('aws-sdk');

exports.handler = function(event, context, callback) {

  var iot = new AWS.Iot();
  var params = {
    attributeName: 'type',
    attributeValue: 'j2534',
    maxResults: 100
  };

  iot.listThings(params, function(err, data) {
    if(err) {
      console.log(err, err.stack);
      context.fail(event); 
    }
    else {
      console.log(data);
      context.succeed(data);
    }
  });

}
