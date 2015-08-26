var redisCollection = new Meteor.RedisCollection("redis");

if (Meteor.isClient) {
    Meteor.subscribe("things");

    Template.thingstemp.helpers({
        things: function(){
            return redisCollection.matching("things-*");
        }
    });

}

if (Meteor.isServer) {
    var net = Npm.require('net');
    var Fiber = Npm.require('fibers');

    Meteor.publish("things", function () {
      console.log(redisCollection.matching("things-*"));
      return redisCollection.matching("things-*");
    });


  Meteor.startup(function () {
    var client = net.createConnection(2000,'192.168.0.35');

    var message = "";
        client.on('data', function(data) {
                // console.log('Received: ' + data);
                message += data;
                if (message.slice(-1) == '\n') {
                    // console.log('message: >' + message+'<');
                    
                    try {
                        var d = JSON.parse(message);
                        Fiber(function() {
                            //redisCollection.set('batteryVoltage',d.batteryVoltage);
                            Object.keys(d).forEach(function(key) {
                                redisCollection.set(key,d[key]);
                            });
                        }).run();
                        console.log(d);
                    } catch(err) {
                        console.log(message);
                    }
                    message = "";
                }
            });
    });
};
