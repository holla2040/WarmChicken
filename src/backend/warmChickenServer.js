var redisCollection = new Meteor.RedisCollection('redis')

if (Meteor.isClient) {
    Meteor.subscribe("warmChickenData");

    Template.wcDataTable.helpers({
        wcKeys: function(){
            return redisCollection.matching("warmChicken.*");
        }
    });

    Template.dashboard.helpers({
        wc: function(){
            var data = Object();
            var d = redisCollection.matching("warmChicken.*").fetch();
            d.forEach(function(o) {
                data[o.key.replace("warmChicken.","")] = o.value;
            });
            return data;
        }
    });

    Template.registerHelper('capitalize', function(v) {
        return s(v).capitalize().value();
        //return v;
    });

    Template.registerHelper('delPath', function(v) {
        return v.replace("warmChicken.","");
    });

    Template.registerHelper('oneDecimal', function(v) {
        return s.numberFormat(v,1);
    });

    Meteor.setInterval(function() {
        var now = new Date().getTime()/1000;
        var d = parseInt(redisCollection.matching("warmChicken.updateTimeSeconds").fetch()[0].value);

        //console.log(now, d, now - d);

        if ((now - d) > 70) { // should have 4 updates in 70 seconds
            $('body').addClass('stale');
        } else {
            $('body').removeClass('stale');
        }
    
    }, 10000);
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
        if (message.indexOf('}') != -1) {
            var updates = message.split("\r");
            updates.forEach(function(update) {
                if (update[0] == '{' && update.slice(-1) == '}') {
                    //console.log(update);
                    try {
                        var d = JSON.parse(update);
                        console.log(JSON.stringify(d, undefined, 2)+"\n");
                        Fiber(function() {
                            //redisCollection.set('batteryVoltage',d.batteryVoltage);
                            Object.keys(d).forEach(function(key) {
                                redisCollection.set("warmChicken."+key,d[key]);
                            });
                            redisCollection.set("warmChicken.updateDay",moment(new Date()).format('YYMMDD'));
                            redisCollection.set("warmChicken.updateTime", moment(new Date()).format('HHmmss'));
                            redisCollection.set("warmChicken.updateTimeSeconds", new Date().getTime()/1000);
                        }).run();
                        // console.log(d);
                    } catch(err) {
                        console.log(err+"\n"+update);
                    }
                }
            });
            message = "";  // this isn't quite right might drop the next message but who cares, its a chicken coop!
        }
    });
    client.on('close',function() {
        console.log('connection closed');
        Meteor.setTimeout(function() {
            client.connect(2000, '192.168.0.35',function() {
                console.log("connection open");
            })
        },5000);
    });

  });
};
