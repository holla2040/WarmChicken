var redisCollection = new Meteor.RedisCollection("redis");

if (Meteor.isClient) {
    Meteor.subscribe("warmChickenData");

    Template.wcDataTable.helpers({
        wcKeys: function(){
            return redisCollection.matching("warmChicken.*");
        }
    });
}

if (Meteor.isServer) {
    var net = Npm.require('net');
    var Fiber = Npm.require('fibers');

    Meteor.publish("warmChickenData", function () {
      return redisCollection.matching("warmChicken.*");
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
                                redisCollection.set("warmChicken."+key,d[key]);
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
